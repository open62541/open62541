import assert from 'node:assert/strict';
import { isDeepStrictEqual } from 'node:util';

export const statusCodes = { good: 0, invalidSession: 0x80250000, tooManySessions: 0x80560000 };
const statusNames = {
  good: 'UA_STATUSCODE_GOOD', invalidSession: 'UA_STATUSCODE_BADSESSIONIDINVALID',
  tooManySessions: 'UA_STATUSCODE_BADTOOMANYSESSIONS',
};
const kinds = { create: 0, activate: 1, close: 2, advance: 3 };
const keys = (value, expected) => assert.deepEqual(Object.keys(value), expected);
const integer = (value, upper) => assert(Number.isSafeInteger(value) && value >= 0 && value <= upper);

export function parseRows(text) {
  assert(text.endsWith('\n'));
  const rows = text.trimEnd().split('\n').map(line => {
    const row = JSON.parse(line);
    assert.equal(JSON.stringify(row), line, 'Noncanonical or duplicate JSON fields');
    keys(row, ['name', 'maximum', 'tcp', 'ops', 'observations']);
    assert.match(row.name, /^[A-Za-z0-9_]+$/);
    integer(row.maximum, 3);
    assert.equal(typeof row.tcp, 'boolean');
    assert(row.ops.length > 0 && row.ops.length <= 12);
    assert.equal(row.observations.length, row.ops.length);
    let attempts = 0;
    row.ops.forEach(op => {
      keys(op, ['kind', 'arg']);
      assert(Object.hasOwn(kinds, op.kind));
      integer(op.arg, op.kind === 'create' ? 60000 : 1000);
      if (op.kind === 'create') {
        assert(op.arg > 0);
        attempts++;
      } else if (op.kind !== 'advance') integer(op.arg, 7);
    });
    assert(attempts <= 8);
    row.observations.forEach(o => {
      keys(o, ['status', 'liveMask', 'activeMask', 'count', 'activeCount']);
      assert(Object.hasOwn(statusCodes, o.status));
      integer(o.liveMask, 255);
      integer(o.activeMask, 255);
      integer(o.count, 8);
      integer(o.activeCount, 8);
      assert.equal(o.activeMask & ~o.liveMask, 0);
    });
    return row;
  });
  assert.equal(rows.length, 2033, 'Missing or extra declared traces');
  assert.equal(new Set(rows.map(row => row.name)).size, rows.length, 'Duplicate trace');
  assert.equal(rows.filter(row => row.tcp).length, 4);
  return rows;
}

export function certificateSource(rows, start = 0) {
  let text = 'import Corpus\nnamespace SessionLifecycle\n';
  rows.forEach((row, offset) => {
    const ops = row.ops.map(op => `.${op.kind} ${op.arg}`).join(', ');
    const scenario = `{ name := "${row.name}", maximum := ${row.maximum}, tcp := ${row.tcp}, ops := [${ops}] }`;
    const observations = row.observations.map(o =>
      `{ status := .${o.status}, liveMask := ${o.liveMask}, activeMask := ${o.activeMask}, count := ${o.count}, activeCount := ${o.activeCount} }`).join(', ');
    text += `theorem trace_${start + offset} : observe ${scenario} = [${observations}] := by rfl\n`;
    text += `#print axioms trace_${start + offset}\n`;
  });
  return text + 'end SessionLifecycle\n';
}

export function auditAxioms(output, expected, allowed = []) {
  assert(!/\bsorry\b|\bsorryAx\b|error:|warning:/i.test(output), 'Incomplete or unsafe proof');
  const audits = [...output.matchAll(/'([^']+)' (?:does not depend on any axioms|depends on axioms: \[([^\]]*)\])/g)];
  assert.equal(audits.length, expected);
  assert.equal(new Set(audits.map(a => a[1])).size, expected);
  for (const audit of audits) {
    const axioms = audit[2] ? audit[2].split(',').map(x => x.trim()) : [];
    assert(axioms.every(axiom => allowed.includes(axiom)), `Unexpected axioms: ${audit[1]}`);
  }
}

export function fixtureHeader(rows) {
  const pool = [];
  const indices = new Map();
  const traceIndices = rows.map(row => row.ops.map((op, index) => {
    const o = row.observations[index];
    const values = [kinds[op.kind], op.arg, statusNames[o.status],
      o.liveMask, o.activeMask, o.count, o.activeCount];
    const key = JSON.stringify(values);
    if(!indices.has(key)) {
      indices.set(key, pool.length);
      pool.push(values);
    }
    return indices.get(key);
  }));
  const text = [
    '/* Generated from kernel-checked Session lifecycle literals. Do not edit. */',
    '#ifndef SESSION_LIFECYCLE_EXPECTATIONS_H',
    '#define SESSION_LIFECYCLE_EXPECTATIONS_H',
    '#define SESSION_MODEL_HANDLES 8',
    `#define SESSION_MODEL_CASES ${rows.length}`,
    'typedef struct {',
    '    unsigned kind, arg;',
    '    UA_StatusCode status;',
    '    unsigned liveMask, activeMask, count, activeCount;',
    '} SessionModelStep;',
    'static const SessionModelStep sessionModelSteps[] = {',
    ...pool.map(values => `    {${values.join(', ')}},`),
    '};',
    'typedef struct {',
    '    const char *name;',
    '    unsigned maximum;',
    '    UA_Boolean tcp;',
    '    unsigned size;',
    '    unsigned steps[12];',
    '} SessionModelCase;',
    'static const SessionModelCase sessionModelCases[SESSION_MODEL_CASES] = {',
  ];
  rows.forEach((row, index) => {
    text.push(`    {"${row.name}", ${row.maximum}, ${row.tcp}, ${row.ops.length}, {${traceIndices[index].join(', ')}}},`);
  });
  return text.concat('};', '#endif', '').join('\n');
}

export function expectedEvents(rows, tcp = true) {
  return (tcp ? ['native', 'tcp'] : ['native']).flatMap(mode => rows.flatMap((row, caseIndex) =>
    mode === 'tcp' && !row.tcp ? [] : row.observations.map((o, step) => ({
      mode, case: caseIndex, step, status: statusCodes[o.status],
      kind: kinds[row.ops[step].kind], arg: row.ops[step].arg,
      liveMask: o.liveMask, activeMask: o.activeMask, count: o.count, activeCount: o.activeCount,
    }))));
}

export function observedEvents(log) {
  return log.split('\n').filter(line => line.startsWith('MODEL_EVENT '))
    .map(line => JSON.parse(line.slice('MODEL_EVENT '.length)));
}

export function failingPrefix(rows, log, tcp = true) {
  const observed = observedEvents(log);
  const expected = new Map(expectedEvents(rows, tcp).map(o => [`${o.mode}:${o.case}:${o.step}`, o]));
  const failed = observed.find(o => !isDeepStrictEqual(o, expected.get(`${o.mode}:${o.case}:${o.step}`)));
  if(!failed || !rows[failed.case]) return null;
  const row = rows[failed.case];
  return {
    name: row.name, maximum: row.maximum, mode: failed.mode, firstFailingStep: failed.step,
    ops: row.ops.slice(0, failed.step + 1), expected: row.observations.slice(0, failed.step + 1),
    observed: observed.filter(o => o.mode === failed.mode && o.case === failed.case),
    scope: 'First-failure prefix, not a globally minimal counterexample.',
  };
}

export function matchEvents(rows, log, tcp = true) {
  const expected = expectedEvents(rows, tcp);
  const observed = observedEvents(log);
  assert.deepEqual(observed, expected, 'Missing, extra, reordered or incorrect native observations');
  const probes = log.split('\n').filter(line => line.startsWith('MODEL_PROBE '))
    .map(line => JSON.parse(line.slice('MODEL_PROBE '.length)));
  const expectedProbes = rows.flatMap((row, caseIndex) => {
    if (!tcp || !row.tcp) return [];
    const final = row.observations.at(-1);
    const count = row.ops.filter(op => op.kind === 'create').length;
    return Array.from({ length: count }, (_, id) => {
      const live = !!(final.liveMask & (1 << id));
      const active = !!(final.activeMask & (1 << id));
      return [
        ...(!active ? [{ case: caseIndex, id, op: 'activate', status: live ? 0 : statusCodes.invalidSession }] : []),
        { case: caseIndex, id, op: 'read', status: live ? 0 : statusCodes.invalidSession },
      ];
    }).flat();
  });
  assert.deepEqual(probes, expectedProbes, 'Incomplete or incorrect TCP token probes');
  return { traces: rows.length, tcpTraces: tcp ? rows.filter(row => row.tcp).length : 0,
    observations: observed.length, tokenProbes: probes.length };
}
