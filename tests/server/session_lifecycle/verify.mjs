#!/usr/bin/env node
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { createHash } from 'node:crypto';
import { mkdir, readFile, stat, writeFile } from 'node:fs/promises';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { performance } from 'node:perf_hooks';
import { auditAxioms, certificateSource, failingPrefix, fixtureHeader, matchEvents, parseRows } from './bridge.mjs';

const root = dirname(fileURLToPath(import.meta.url));
const source = resolve(root, '../../..');
const args = process.argv.slice(2);
const option = (name, fallback) => {
  const index = args.indexOf(name);
  if(index < 0) return fallback;
  assert(index + 1 < args.length && !args[index + 1].startsWith('--'), `Missing ${name} value`);
  return args[index + 1];
};
const build = resolve(option('--build', join(source, 'build-session-model')));
const runDir = resolve(option('--evidence', join(build, 'evidence', new Date().toISOString().replace(/[:.]/g, '-'))));
const lean = option('--lean-bin', 'lean');
const refresh = args.includes('--refresh');
const phases = [];
const started = performance.now();
const receipt = { started: new Date().toISOString(), status: 'RUNNING', source, build, lean, phases };
const hash = bytes => createHash('sha256').update(bytes).digest('hex');
await mkdir(dirname(runDir), { recursive: true });
await mkdir(runDir, { recursive: false });

async function command(name, program, commandArgs, options = {}) {
  const start = performance.now();
  const result = await new Promise((fulfill, reject) => {
    const env = { ...process.env, ...options.env };
    delete env.SESSION_MODEL_CASE;
    delete env.SESSION_MODEL_NATIVE_ONLY;
    const child = spawn(program, commandArgs, { cwd: source, env, stdio: ['ignore', 'pipe', 'pipe'] });
    const chunks = [];
    let bytes = 0;
    const timer = setTimeout(() => child.kill('SIGKILL'), options.timeout ?? 300000);
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
  phases.push({ name, seconds: (performance.now() - start) / 1000, exitCode: result.code, signal: result.signal });
  await writeFile(join(runDir, `${name}.log`), result.text);
  if(!options.allowFailure) assert.equal(result.code, 0, `${name} failed: ${join(runDir, name + '.log')}`);
  return result;
}

const leanArgs = ['-j1', '-s1024', '-DElab.async=false'];
const proof = (name, extra, allowFailure = false) =>
  command(name, 'prlimit', ['--as=1073741824', '--cpu=45:46', '--core=0', '--', lean, ...leanArgs, ...extra],
    { env: { LEAN_PATH: runDir, LEAN_NUM_THREADS: '1', MALLOC_ARENA_MAX: '1' }, timeout: 60000, allowFailure });

let rows;
try {
  receipt.revision = (await command('revision', 'git', ['rev-parse', 'HEAD'])).text.trim();
  receipt.hashes = {};
  for(const file of ['Model.lean', 'Corpus.lean', 'Emit.lean', 'bridge.mjs', 'bridge.test.mjs', 'verify.mjs', 'lean-toolchain']) {
    const bytes = await readFile(join(root, file));
    receipt.hashes[file] = hash(bytes);
    if(file.endsWith('.lean'))
      assert(!/\b(sorry|admit|native_decide|unsafe|axiom)\b/.test(bytes.toString()), `Unapproved proof escape: ${file}`);
  }
  for(const file of ['src/server/ua_services_session.c', 'tests/server/check_session_lifecycle.c', 'tests/CMakeLists.txt'])
    receipt.hashes[file] = hash(await readFile(join(source, file)));
  const version = await command('lean-version', lean, ['--version']);
  assert.match(version.text, /version 4\.19\.0,/);
  const model = await proof('model', ['-o', join(runDir, 'Model.olean'), join(root, 'Model.lean')]);
  auditAxioms(model.text, 17, ['propext', 'Quot.sound']);
  const corpus = await proof('corpus', ['-o', join(runDir, 'Corpus.olean'), join(root, 'Corpus.lean')]);
  auditAxioms(corpus.text, 1, ['propext']);
  const emitted = await proof('json', ['--run', join(root, 'Emit.lean'), 'json']);
  rows = parseRows(emitted.text);
  await writeFile(join(runDir, 'corpus.jsonl'), emitted.text);
  const batchSize = 128;
  for(let start = 0; start < rows.length; start += batchSize) {
    const batch = rows.slice(start, start + batchSize);
    const module = await proof(`emit-${start}`, ['--run', join(root, 'Emit.lean'), 'lean', String(start), String(batch.length)]);
    assert.equal(module.text, certificateSource(batch, start), 'JSON/literal mismatch');
    const path = join(runDir, `Certificates${start}.lean`);
    await writeFile(path, module.text);
    const checked = await proof(`certify-${start}`, [path]);
    auditAxioms(checked.text, batch.length, ['propext']);
  }
  const falsePath = join(runDir, 'FalseCertificate.lean');
  const falseLiteral = certificateSource(rows.slice(0, 1)).replace('liveMask := 1', 'liveMask := 2');
  assert.notEqual(falseLiteral, certificateSource(rows.slice(0, 1)));
  await writeFile(falsePath, falseLiteral);
  const rejected = await proof('false-certificate-control', [falsePath], true);
  assert(rejected.code !== 0 && rejected.code !== null && rejected.signal === null);
  assert.match(rejected.text, /type mismatch|tactic.*failed|unsolved goals/);
  await command('bridge-controls', process.execPath, [join(root, 'bridge.test.mjs')]);

  const header = fixtureHeader(rows);
  const target = join(source, 'tests/server/session_lifecycle_expectations.h');
  if(refresh) await writeFile(target, header);
  else assert.equal(await readFile(target, 'utf8'), header, 'Stale fixture; review and regenerate with --refresh');
  receipt.hashes.fixture = hash(header);
  receipt.proofs = { modelTheorems: 18, literalCertificates: rows.length };
  if(!args.includes('--proof-only')) {
    const dependencies = option('--dependencies', '');
    const env = dependencies ? {
      PKG_CONFIG_SYSROOT_DIR: resolve(dependencies),
      PKG_CONFIG_LIBDIR: join(resolve(dependencies), 'usr/lib/x86_64-linux-gnu/pkgconfig'),
      LD_LIBRARY_PATH: join(resolve(dependencies), 'usr/lib/x86_64-linux-gnu'),
    } : {};
    const multithreading = option('--multithreading', '100');
    assert(['0', '100'].includes(multithreading));
    const tcp = multithreading === '100';
    receipt.configuration = { buildType: 'Debug', encryption: 'OFF', namespace: 'REDUCED', multithreading, tcpReplay: tcp };
    const extraFlags = option('--c-flags', '');
    await command('configure', 'cmake', ['-S', source, '-B', build, '-G', 'Ninja',
      '-DCMAKE_BUILD_TYPE=Debug', '-DUA_BUILD_UNIT_TESTS=ON', '-DUA_FORCE_WERROR=ON',
      '-DUA_NAMESPACE_ZERO=REDUCED', '-DUA_ENABLE_ENCRYPTION=OFF', `-DUA_MULTITHREADING=${multithreading}`,
      `-DCMAKE_C_FLAGS=-I${join(source, 'plugins')} ${extraFlags}`], { env });
    await command('build', 'cmake', ['--build', build, '--target', 'check_session_lifecycle', '--parallel', '3'], { env });
    const nativeArgs = ['--test-dir', build, '-R', '^check_session_lifecycle$', '--output-on-failure',
      '--no-tests=error', '--timeout', '240', '--output-junit', join(runDir, 'ctest.xml')];
    const nativeStarted = Date.now();
    const native = args.includes('--isolate-network') ?
      await command('native', 'unshare', ['--user', '--map-root-user', '--net', 'bash',
        join(root, 'network.sh'), 'ctest', ...nativeArgs], { env, timeout: 250000, allowFailure: true }) :
      await command('native', 'ctest', nativeArgs, { env, timeout: 250000, allowFailure: true });
    const logPath = join(build, 'Testing/Temporary/LastTest.log');
    const info = await stat(logPath).catch(() => null);
    assert(info && info.mtimeMs >= nativeStarted - 1000,
      `Native run produced no fresh log; inspect ${join(runDir, 'native.log')}`);
    const log = await readFile(logPath, 'utf8');
    await writeFile(join(runDir, 'native-full.log'), log);
    if(native.code !== 0) {
      const prefix = failingPrefix(rows, log, tcp);
      if(prefix) {
        prefix.replay = prefix.mode === 'native' ?
          `SESSION_MODEL_CASE=${prefix.name} SESSION_MODEL_NATIVE_ONLY=1 ${join(build, 'bin/tests/check_session_lifecycle')}` :
          `ctest --test-dir ${build} -R '^check_session_lifecycle$' --output-on-failure`;
        await writeFile(join(runDir, 'failing-prefix.json'), JSON.stringify(prefix, null, 2) + '\n');
      }
      assert.equal(native.code, 0, `Native failure; inspect ${runDir}`);
    }
    assert(log.includes(`100%: Checks: ${tcp ? 2 : 1}, Failures: 0, Errors: 0`));
    receipt.result = matchEvents(rows, log, tcp);
  }
  receipt.status = args.includes('--proof-only') ? 'PROOF_AND_FIXTURE_PASS' : 'PASS';
} catch(error) {
  receipt.status = 'FAIL';
  receipt.error = error.stack ?? String(error);
  process.exitCode = 1;
  console.error(receipt.error);
} finally {
  receipt.seconds = (performance.now() - started) / 1000;
  await writeFile(join(runDir, 'receipt.json'), JSON.stringify(receipt, null, 2) + '\n');
  console.log(`${receipt.status}: ${receipt.seconds.toFixed(2)} s; ${runDir}`);
}
