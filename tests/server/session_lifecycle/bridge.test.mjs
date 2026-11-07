import assert from 'node:assert/strict';
import test from 'node:test';
import { auditAxioms, certificateSource, expectedEvents, failingPrefix, matchEvents, observedEvents, parseRows } from './bridge.mjs';

test('reject incomplete and unsafe axiom audits', () => {
  assert.throws(() => auditAxioms('', 1));
  assert.throws(() => auditAxioms("'x' depends on axioms: [sorryAx]", 1));
  assert.throws(() => auditAxioms("'x' depends on axioms: [customTrust]", 1));
});
test('literal data is not silently recomputed', () => {
  const row = { name: 'example', maximum: 1, tcp: false,
    ops: [{ kind: 'create', arg: 20 }],
    observations: [{ status: 'good', liveMask: 1, activeMask: 0, count: 1, activeCount: 0 }] };
  const original = certificateSource([row]);
  row.observations[0].liveMask = 2;
  assert.notEqual(certificateSource([row]), original);
});
test('reject empty and noncanonical corpus', () => {
  assert.throws(() => parseRows('\n'));
  assert.throws(() => parseRows('{"name":"x","name":"y"}\n'));
});
test('all required observations remain ordered', () => {
  const rows = [{ tcp: false, ops: [{kind: 'create', arg: 20}, {kind: 'activate', arg: 0}], observations: [
    { status: 'good', liveMask: 1, activeMask: 0, count: 1, activeCount: 0 },
    { status: 'good', liveMask: 1, activeMask: 1, count: 1, activeCount: 1 },
  ] }];
  const expected = expectedEvents(rows);
  const log = expected.map(event => 'MODEL_EVENT ' + JSON.stringify(event)).join('\n');
  assert.deepEqual(observedEvents(log), expected);
  assert.doesNotThrow(() => matchEvents(rows, log));
  assert.throws(() => matchEvents(rows, log.split('\n')[0]));
  assert.throws(() => matchEvents(rows, log + '\n' + log));
  assert.throws(() => matchEvents(rows, log.split('\n').reverse().join('\n')));
  assert.throws(() => matchEvents(rows, log.replace('"status":0', '"status":1')));
});
test('reject missing, duplicate, and out-of-range rows', () => {
  const rows = Array.from({length: 2033}, (_, id) => ({
    name: `case_${id}`, maximum: 1, tcp: id < 4, ops: [{kind: 'create', arg: 20}],
    observations: [{status: 'good', liveMask: 1, activeMask: 0, count: 1, activeCount: 0}],
  }));
  const text = list => list.map(row => JSON.stringify(row)).join('\n') + '\n';
  assert.doesNotThrow(() => parseRows(text(rows)));
  assert.throws(() => parseRows(text(rows.slice(1))));
  rows[1].name = rows[0].name;
  assert.throws(() => parseRows(text(rows)));
  rows[1].name = 'case_1';
  rows[0].observations[0].liveMask = 256;
  assert.throws(() => parseRows(text(rows)));
});
test('failure prefix selects the first bad observation, not the final TCP event', () => {
  const row = { name: 'native_example', maximum: 1, tcp: true,
    ops: [{kind: 'create', arg: 20}, {kind: 'activate', arg: 0}],
    observations: [
      {status: 'good', liveMask: 1, activeMask: 0, count: 1, activeCount: 0},
      {status: 'good', liveMask: 1, activeMask: 1, count: 1, activeCount: 1},
    ] };
  const events = expectedEvents([row]);
  const text = list => list.map(event => 'MODEL_EVENT ' + JSON.stringify(event)).join('\n');
  assert.equal(failingPrefix([row], text(events)), null);
  events[1].activeMask = 0;
  const prefix = failingPrefix([row], text(events));
  assert.equal(prefix.mode, 'native');
  assert.equal(prefix.firstFailingStep, 1);
  assert.deepEqual(prefix.ops, row.ops);
});
