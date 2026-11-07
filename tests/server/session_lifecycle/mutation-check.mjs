#!/usr/bin/env node
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { performance } from 'node:perf_hooks';
import { failingPrefix, fixtureHeader, parseRows } from './bridge.mjs';

const root = dirname(fileURLToPath(import.meta.url));
const source = resolve(root, '../../..');
const args = process.argv.slice(2);
const option = (name, fallback) => {
  const index = args.indexOf(name);
  if(index < 0) return fallback;
  assert(args[index + 1] && !args[index + 1].startsWith('--'));
  return args[index + 1];
};
const build = resolve(option('--build', join(source, 'build-session-model')));
const evidence = resolve(option('--evidence', join(build, 'mutation-' + Date.now())));
const file = join(source, 'src/server/ua_services_session.c');
const original = await readFile(file, 'utf8');
const corpusPath = option('--corpus', '');
const rows = corpusPath ? parseRows(await readFile(resolve(corpusPath), 'utf8')) : null;
if(rows) assert.equal(await readFile(join(source, 'tests/server/session_lifecycle_expectations.h'), 'utf8'),
  fixtureHeader(rows), 'Corpus does not match the current fixture');
const sha = text => createHash('sha256').update(text).digest('hex');
const predicate = 'if(sentry->session.state == UA_SESSIONSTATE_CREATE_REQUESTED ||\n' +
  '           sentry->session.state == UA_SESSIONSTATE_CREATED)';
const mutations = [
  ['newest_pending', '            oldest = sentry;', '            { oldest = sentry; break; }'],
  ['active_victim', predicate, 'if(true)'],
  ['activation_stays_pending', '        session->state = UA_SESSIONSTATE_ACTIVATED;',
    '        session->state = UA_SESSIONSTATE_CREATED;'],
  ['close_keeps_record', '    UA_Session_remove(server, session, UA_SHUTDOWNREASON_CLOSE);', '    (void)session;'],
  ['expiry_at_equal_deadline', 'if(sentry->session.validTill >= nowMonotonic)', 'if(sentry->session.validTill > nowMonotonic)'],
  ['activation_does_not_refresh_deadline', '    UA_Session_updateLifetime(session, now, nowMonotonic);',
    '    (void)now;'],
  ['zero_is_not_unlimited', 'server->config.maxSessions != 0 &&', 'true &&'],
];
await mkdir(evidence, { recursive: false });

async function command(name, program, parameters, options = {}) {
  const started = performance.now();
  const result = await new Promise((fulfill, reject) => {
    const env = { ...process.env, ...options.env };
    delete env.SESSION_MODEL_CASE;
    const child = spawn(program, parameters, { cwd: source, env, stdio: ['ignore', 'pipe', 'pipe'] });
    const chunks = [];
    let bytes = 0;
    const timer = setTimeout(() => child.kill('SIGKILL'), 180000);
    const collect = chunk => {
      bytes += chunk.length;
      if(bytes > 12000000) child.kill('SIGKILL');
      else chunks.push(chunk);
    };
    child.stdout.on('data', collect);
    child.stderr.on('data', collect);
    child.on('error', error => { clearTimeout(timer); reject(error); });
    child.on('close', (code, signal) => {
      clearTimeout(timer);
      fulfill({ code, signal, text: Buffer.concat(chunks).toString('utf8') });
    });
  });
  await writeFile(join(evidence, `${name}.log`), result.text);
  return { ...result, seconds: (performance.now() - started) / 1000 };
}

const receipt = { sourceHash: sha(original), mutations: [], status: 'RUNNING' };
try {
  const clean = await command('source-clean', 'git', ['diff', '--exit-code', '--', 'src/server/ua_services_session.c']);
  assert.equal(clean.code, 0, 'Refusing to mutate an already modified production file');
  const binary = join(build, 'bin/tests/check_session_lifecycle');
  const baseline = await command('baseline', binary, [], { env: { SESSION_MODEL_NATIVE_ONLY: '1' } });
  assert.equal(baseline.code, 0);
  assert.match(baseline.text, /100%: Checks: 1, Failures: 0, Errors: 0/);
  for(const [name, before, after] of mutations) {
    assert.equal(original.split(before).length, 2, `Changed mutation anchor: ${name}`);
    assert.equal(await readFile(file, 'utf8'), original, 'Unexpected production edit');
    await writeFile(file, original.replace(before, after));
    try {
      const built = await command(`${name}-build`, 'cmake', ['--build', build, '--target', 'check_session_lifecycle', '--parallel', '3']);
      assert.equal(built.code, 0, `Mutation must compile: ${name}`);
      const checked = await command(name, binary, [], { env: { SESSION_MODEL_NATIVE_ONLY: '1' } });
      assert(checked.code !== 0 && checked.code !== null && checked.signal === null, `Mutation was not rejected: ${name}`);
      assert.match(checked.text, /case=\d+ step=\d+:/, 'Fault must fail a lifecycle observation assertion');
      if(rows) {
        const prefix = failingPrefix(rows, checked.text, false);
        assert(prefix, 'Missing semantic counterexample');
        prefix.replay = `SESSION_MODEL_CASE=${prefix.name} SESSION_MODEL_NATIVE_ONLY=1 ${binary}`;
        await writeFile(join(evidence, `${name}-prefix.json`), JSON.stringify(prefix, null, 2) + '\n');
      }
      receipt.mutations.push({ name, seconds: checked.seconds, exitCode: checked.code,
        failure: checked.text.match(/.*case=\d+ step=\d+:.*$/m)?.[0] });
      console.log(`${name}: behavioral rejection confirmed`);
    } finally {
      assert.equal(await readFile(file, 'utf8'), original.replace(before, after),
        'Concurrent source change; preserving it instead of restoring');
      await writeFile(file, original);
    }
  }
  receipt.status = 'PASS';
} catch(error) {
  receipt.status = 'FAIL';
  receipt.error = error.stack ?? String(error);
  process.exitCode = 1;
  console.error(receipt.error);
} finally {
  assert.equal(sha(await readFile(file)), sha(original), 'Production source not restored');
  const restoredBuild = await command('restored-build', 'cmake', ['--build', build, '--target', 'check_session_lifecycle', '--parallel', '3']);
  const restored = await command('restored-native', join(build, 'bin/tests/check_session_lifecycle'), [],
    { env: { SESSION_MODEL_NATIVE_ONLY: '1' } });
  if(restoredBuild.code !== 0 || restored.code !== 0) {
    receipt.status = 'FAIL';
    process.exitCode = 1;
  }
  receipt.restoredSourceHash = sha(await readFile(file));
  receipt.restoredExitCode = restored.code;
  await writeFile(join(evidence, 'receipt.json'), JSON.stringify(receipt, null, 2) + '\n');
  console.log(`${receipt.status}: ${evidence}`);
}
