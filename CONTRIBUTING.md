# Contributing to open62541

Contributions to open62541 include code, documentation, testing, issue reports,
answers to user questions, and work on project infrastructure. We welcome
contributions made in good faith and value improvements of every size.

This guide covers contributions to the core open62541 repository. If anything
is unclear, open an issue or ask in the relevant pull request.

## Code of Conduct

All contributors must follow the project [Code of Conduct](CODE_OF_CONDUCT.md).

## Before You Start

Search existing issues and pull requests before beginning substantial work.
Discuss large features, architectural changes, new dependencies, and public API
changes with the maintainers early. This avoids duplicate work and allows the
design to be reviewed before much code is written.

Do not open a public issue or pull request for a suspected vulnerability.
Follow [SECURITY.md](SECURITY.md) to report it privately.

## Pull Requests

Anyone may open a pull request. A maintainer reviews and merges it.

Keep each pull request focused on one change. Separate unrelated refactoring,
formatting, generated output, and dependency updates. Small, self-contained
changes are easier to understand, test, and review. Use a draft pull request if
you want early feedback on unfinished work.

A pull request should:

- explain the problem and the chosen solution;
- identify important behavior, compatibility, or configuration effects;
- include tests for new behavior and relevant failure cases;
- update affected documentation and `CHANGES.md` where appropriate;
- avoid unrelated changes; and
- pass the required continuous integration checks.

Link related issues. Include enough context for a reviewer to understand the
change without reconstructing the reasoning from the commit history.

Respond to review comments and keep the branch current with its target branch.
Prefer rebasing over merge commits in the pull request branch. If a pull request
has received no response after a reasonable time, a polite follow-up is
welcome.

Before a contributor's first pull request can be merged, the contributor must
sign the [Contributor License Agreement](https://cla-assistant.io/open62541/open62541).
The signing link is added automatically to the pull request.

## Security-Sensitive Changes

Treat data from networks, files, public APIs, callbacks, and plug-ins as
untrusted. Validate it before it controls memory access, allocation, iteration,
dispatch, security decisions, or persistent state. Check bounds, arithmetic,
conversions, ownership, resource use, error paths, authentication, and
authorization. Use existing OPC UA security mechanisms and maintained
cryptographic backends. Add tests for intended behavior and relevant failure
cases. If you are unsure, mention the possible security effect in the pull
request. Report suspected vulnerabilities privately through
[SECURITY.md](SECURITY.md).

Use the [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
as guidance. Continuous integration runs the supported CERT checks so that
detected rule violations are explicit in code review. The checks do not cover
every CERT rule. Fix a finding or use the narrowest supported suppression with
a short explanation for the reviewer.

## Changes to the Public API

Headers under `include/` define the public API. Discuss public API changes with
the maintainers before implementation.

Preserve source and binary compatibility where practical. Prefer adding a new
API and deprecating the old one with `UA_DEPRECATED` over changing or removing
an existing API. A breaking API change requires the `breaking` commit type and
an entry in `CHANGES.md`.

Document ownership, lifetime, thread-safety, callback, and error-handling
expectations for new public APIs.

## Commits and Pull Request Titles

Use this Conventional Commits-style header for commit subjects and pull
request titles:

```text
<type>(<scope>): <subject>
```

The scope is optional. Use the component a reader of the changelog would
recognize, such as `client`, `server`, `core`, `pubsub`, `plugin`, `arch`,
`build`, `deps`, `ci`, `tests`, or `sec`.

Use one of these types:

- `build`: build system or dependency changes
- `breaking`: changes that break an existing public API
- `ci`: continuous integration changes
- `docs`: documentation-only changes
- `feat`: new functionality
- `fix`: bug fixes
- `perf`: performance improvements
- `refactor`: code changes without a feature or bug fix
- `style`: formatting or other non-functional changes
- `test`: test additions or corrections

Write the subject in the imperative present tense, begin it with a lowercase
letter, and omit the final period. Keep every commit-message line at or below
100 characters.

Use the body to explain why the change is needed and any behavior that is not
obvious from the diff. Reference affected issues in the footer when applicable.

Examples:

```text
docs(server): clarify lifecycle callback ownership
```

```text
fix(core): reject an out-of-range message length

Validate the decoded length before using it to advance the input buffer.
```

Keep commits coherent. Fixup commits may be useful during review, but squash
them before merge when they do not add lasting value to the history.

## Code Style

Follow the style of the surrounding code. In particular:

- indent with four spaces, not tabs;
- put opening braces on the same line as the statement and closing braces on a
  separate line;
- do not add a space before parentheses in control statements or function
  calls;
- place spaces around binary operators, but not unary or member-access
  operators;
- write pointer declarations as `const int *value`;
- use `/* ... */` comments in C source and `/** ... */` for public API
  documentation;
- keep comments focused on purpose, assumptions, and non-obvious constraints;
- avoid unexplained numeric constants and unnecessary global state; and
- prefer clear, focused functions over deeply nested or duplicated code.

An `if` statement with multiple branches is formatted as follows:

```c
if(a) {
    doA();
} else if(b) {
    doB();
} else {
    doC();
}
```

Do not reformat unrelated code in a functional change.

### Names and Declarations

- Public identifiers begin with `UA_`.
- Public server and client functions normally begin with `UA_Server_` and
  `UA_Client_` respectively.
- Functions and variables use lower camel case, such as `calculateSize`.
- Types use upper camel case, such as `UA_ResponseHeader`.
- Constants, enumeration values, and macros use uppercase names, such as
  `UA_STATUSCODE_GOOD`.
- Do not introduce identifiers reserved by the C implementation, including
  names beginning with a double underscore.
- Use descriptive names. Familiar short names such as loop indices are fine
  when their meaning is clear.
- Do not use Hungarian notation.
- Mark variables and input parameters `const` when they are not modified.
- Place input parameters before output parameters.

Use the existing initialization and cleanup functions for open62541 types. Make
ownership transfers clear and ensure every acquired resource is released
exactly once.

## Error Handling and Cleanup

Check every result that can report failure. Preserve the original error unless
the caller needs a more appropriate open62541 status code. Log only when the
current layer adds useful context; avoid logging the same failure repeatedly at
several layers.

Internal code provides `UA_CHECK`, `UA_CHECK_STATUS`, and `UA_CHECK_MEM`, plus
logging variants. Use them when they make the success path and cleanup easier
to read. A direct conditional is equally appropriate when special handling is
needed.

Use a cleanup label when multiple paths release the same resources:

```c
static UA_StatusCode
performWork(void) {
    void *data = UA_malloc(128);
    UA_CHECK_MEM(data, return UA_STATUSCODE_BADOUTOFMEMORY);

    UA_StatusCode res = firstStep(data);
    UA_CHECK_STATUS(res, goto cleanup);

    res = secondStep(data);
    UA_CHECK_STATUS(res, goto cleanup);

cleanup:
    UA_free(data);
    return res;
}
```

Avoid a cleanup label when a direct return is clearer and does not duplicate
cleanup logic.

## Tests and Automated Checks

Add or update tests for behavior changed by the pull request. A bug fix should
normally include a regression test that fails without the fix. Cover relevant
boundaries, invalid input, failure paths, cleanup, and configuration variants.

Unit tests are under `tests/` and conventionally use filenames beginning with
`check_`. A typical local test build is:

```sh
cmake -S . -B build -DUA_BUILD_UNIT_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the tests relevant to the change locally. Continuous integration builds
additional compilers, platforms, options, and analysis configurations. All
required checks must pass before merge.

When changing generated code, modify the generator or authoritative input and
regenerate the output. Do not hand-edit generated files unless the repository
explicitly treats them as authoritative source.

## Documentation

Update documentation when a change affects public APIs, configuration,
behavior, compatibility, security assumptions, or deployment. Keep examples
small enough to copy and run, and verify that names and options match the code.

Public declarations under `include/` use documentation comments beginning with
`/**`. Explain constraints and ownership that callers must know; do not merely
repeat the function signature.

## Getting Help

If you are unsure about design, code style, testing, or compatibility, ask in
an issue or pull request. For possible vulnerabilities, use the private
reporting channels in [SECURITY.md](SECURITY.md) instead.
