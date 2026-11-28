# Model-based Session lifecycle regression tests

This optional companion checks a small Session model against real open62541
code. Normal CTest needs only the committed generated C fixture, not Lean or
Node.js. The companion regenerates and checks that fixture independently.

Successful creation alone does not establish correct reclamation. The histories
also check which authentication tokens remain present, which records are
activated, and whether Session counters agree after each operation. This detects
a wrong victim even if CreateSession still returns success.

## Standards And Provenance

Starting point: OPC 10000-4, Part 4: Services, v1.05.07, 5.7.2.1:
https://reference.opcfoundation.org/specs/OPC-10000-4/5.7.2.1

The selected clause discusses initial activation, reclamation of the oldest
unactivated Session, lifetime, and explicit close. Interpretation and
formalization were AI/LLM-assisted, with source comparison. Human normative
review is still required; proof checking does not validate that interpretation.
The model, proof scripts, and harness were drafted with LLM assistance, but
acceptance uses checked proofs and native agreement, not LLM confidence.
The combined verifier does not call an LLM at test time.

Creation-age ordering and reclamation at the configured limit are explicit
policy choices. Zero-as-unlimited and retention at the exact expiry instant
are implementation contracts, not claims extracted directly from the prose.
The model assumes positive requested timeouts accepted without revision.

`Model.lean` contains the abstract transitions and invariants. `Corpus.lean`
enumerates 25 initial population/activation combinations at capacities 1, 2,
and 3, followed by every two-operation word over its nine-operation alphabet.
Eight longer selected histories add transition interactions, reversed timeout
order, deadline refresh, exact expiry, and unlimited capacity.

This is 2,033 declared histories, not exhaustive OPC UA testing. The generator's
coverage of the full standard is not proved. Each history starts empty:
the initial create/activate operations are executed, not assumed.

Generated names follow
`c{limit}_n{setup Sessions}_m{activated-setup bitmask}_w{first op}_{second op}`.
The last two indices refer to the alphabet in `Corpus.lean`; descriptive names
identify the eight selected longer histories.

## What the logic establishes

There are 17 transition/supporting properties in `Model.lean` and one
observation-count property in `Corpus.lean`, not 18 separately extracted
standard requirements. The model proves oldest-pending selection, removal of
exactly one eligible victim, preservation of existing activated records during
creation, and preservation of existing records on capacity refusal.
Activation preserves population size and changes reclamation eligibility;
close removes the matching identity; expiry removes past-deadline records and
retains exact-deadline records.

The capacity theorem applies to any finite model history under a fixed positive
limit and an initially bounded population, not only the enumerated capacities
and trace lengths. Unlimited creation has a separate no-eviction property.
Refusal still consumes an abstract attempted-creation handle; it preserves the
Session list, not every bookkeeping field.

Each literal certificate checks the complete expected observation sequence
against the executable model. Deadline refresh is defined in the activation
transition and checked by concrete histories, without a separate generic
deadline-refresh theorem. Agreement with C is established for executed
histories, not proved for all possible C executions.

## One Command

Requirements: Linux, Lean 4.19.0, Node.js 22 or newer, CMake, Ninja, a C compiler,
Check, Subunit, and the ordinary open62541 test dependencies. The driver uses
Linux `prlimit`; it installs nothing and does not access the network.

From the repository root:

```sh
node tests/server/session_lifecycle/verify.mjs --build build-session-model
```

The same driver is available as `lake test` from this companion directory;
pass verifier options after `--`. Both entry points recheck proofs rather
than treating previously compiled model files as a verdict.

Lean must resolve to the pinned version. For an explicit executable:

```sh
node tests/server/session_lifecycle/verify.mjs \
  --lean-bin /path/to/lean-4.19.0/bin/lean \
  --build build-session-model
```

Lean 4.19.0 is the tested reproducibility pin, not a newest/minimum-version
claim. Other toolchain versions have not been qualified for this contribution.

The driver rechecks the model properties and every literal history equation
in fresh proof directories. It audits dependencies, compares the exact checked
statements with the emitted JSON, verifies the committed C fixture, builds the
real test target, and rejects missing, extra, reordered, or wrong observations.
The bridge only serializes data; it does not select victims.

`--refresh` updates the fixture only after successful proof checking. Review
and commit that generated change. Normal runs refuse stale fixtures.
To regenerate and run the complete checks:

```sh
node tests/server/session_lifecycle/verify.mjs --build build-session-model --refresh
```

Ordinary CTest does not check freshness against the model. The optional GitHub
workflow is manual-only, not an automatic PR freshness gate. For a legitimate
behavior change, review the intended contract, update the model if appropriate,
regenerate, and rerun native tests before accepting the generated diff.

`--proof-only` omits the native build and returns a distinct proof-only verdict.
`--isolate-network` uses an unprivileged isolated loopback namespace where the
host supports it. `--multithreading 0` selects the non-locking configuration
and the in-process corpus only; threaded TCP replay requires 100, the default.
The receipt records this distinction. `--c-flags` supplies instrumentation.

Every run gets a fresh evidence directory under its build directory; use
`--evidence NEW_DIRECTORY` to choose another one. Receipts record tool version,
source revision, input hashes, phase times, and verdict. Production changes
are allowed and tested, not rejected solely because their hash changed.

## Native Boundaries

The fast corpus uses production `UA_Session_create`, `Service_ActivateSession`,
`Service_CloseSession`, and `cleanupSessions`, with a test SecureChannel using
SecurityPolicy None. It observes actual token lookup, activation state, and
the server's Session counters after each operation. It does not directly set
activation flags or reproduce the reclamation algorithm.

Clock advancement uses the existing fake clock and explicitly invokes
production cleanup. It tests cleanup behavior, not scheduling of the periodic
cleanup callback. Activation refreshes deadlines in the model and real code.
Snapshots inspect Session records and counters without sending another service
request.

Four longer histories also run over real loopback TCP. Raw CreateSession,
ActivateSession, and CloseSession requests bypass automatic client Session
recovery. Final token probes require evicted/closed tokens to fail and survivors
to activate/read successfully. These final probes intentionally mutate pending
survivors only after the modeled history has ended.

TCP histories do not advance the fake clock. They check the service boundary,
not the expiry scheduler or encrypted authentication. The in-process and TCP
boundaries are distinct and neither establishes a proof of the other.

The in-process corpus contributes 9,697 step observations. Locking builds add
four TCP histories of lengths 8, 9, 6, and 7: 30 more observations, or 9,727
total, plus 35 final token-usability probes. Non-locking builds exclude those
threaded replays; the probes are counted separately from step observations.

## Failures And Controls

Every run checks a deliberately false literal and bridge rejection controls.
Timeouts, process signals, truncated output, and incomplete checks fail closed.
Only the usual Lean `propext`/`Quot.sound` dependencies are allowed where used;
`sorry`, custom axioms, `unsafe`, and `native_decide` are rejected.

Native assertion failures identify the trace and first failing step. The driver
saves a replayable first-failure prefix where available. This is not a globally
minimal counterexample or a general delta-debugger.

To replay one native history after configuring/building:

```sh
SESSION_MODEL_CASE=activation_refusal_close_readmission \
SESSION_MODEL_NATIVE_ONLY=1 build-session-model/bin/tests/check_session_lifecycle
```

Single-history/native-only modes are diagnostic and cannot satisfy the complete
driver's observation checks. A nonexistent trace fails rather than passing
without executing anything.

Optional source-fault campaign, on a clean production file after a normal build:

```sh
node tests/server/session_lifecycle/mutation-check.mjs --build build-session-model
```

Add `--corpus PATH_TO_EVIDENCE/corpus.jsonl` from a successful verifier run to
save replayable first-failure prefixes for the injected faults.

This temporarily mutates the isolated checkout's production Session file,
requires seven compiled faults to fail behavioral assertions, and restores the
original bytes and native binary. Do not run other builds or edit that source
concurrently. It refuses an initially modified production file. The ordinary
verifier never modifies production code.

The local fault campaign on master
`8f169f1a5adef49220e763c1fb23294ebe412c3f` compiled and rejected all seven faults
below. Operation numbers are one-based and include setup. These are the first
reported failures in those histories, not globally minimized counterexamples.

| Injected fault | History | First failing operation |
| --- | --- | --- |
| Newest pending victim | `c2_n1_m0_w0_0` | 3: Create C after A and B |
| Activated victim allowed | `c1_n1_m0_w1_0` | 3: Create B after activating A |
| Activation leaves a pending record | `c1_n0_m0_w0_1` | 2: Activate A |
| Close leaves the record present | `c1_n0_m0_w0_4` | 2: Close A |
| Expiry at the exact deadline | `c1_n0_m0_w0_8` | 2: Advance to A's deadline |
| Activation does not refresh lifetime | `activation_refreshes_deadline` | 5: Cleanup at the refreshed deadline |
| Zero limit treated as zero capacity | `unlimited_capacity` | 1: Create A |

## What Is Not Proved

Lean proves properties of the explicit model. Native executions establish
agreement for the declared histories and configurations, not arbitrary-history
C refinement, the adapter/compiler's correctness, or complete OPC UA
conformance. Allocation failures, asynchronous callbacks, channel rebinding,
subscriptions, concurrent operations, and runtime reductions of the Session
limit are outside this module. A specification misunderstanding shared by the
model and implementation could survive these checks.
