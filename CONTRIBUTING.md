# Contributing to open62541

Contributions to open62541 include code, documentation, answering user
questions, running the project's infrastructure, and advocating for all types of
open62541 users.

The open62541 project welcomes all contributions from anyone willing to work in
good faith with other contributors and the community. No contribution is too
small and all contributions are valued.

This guide explains the process for contributing to the open62541 project's core
repository and describes what to expect at each step. Thank you for considering
these point.

Your friendly open62541 community!

## Code of Conduct

The open62541 project has a [Code of Conduct](./CODE_OF_CONDUCT.md) that *all*
contributors are expected to follow. This code describes the *minimum* behavior
expectations for all contributors.

## Issues

- Help us help you
- Use the [Issue Template](./.github/ISSUE_TEMPLATE)

## Pull Requests

Everybody can propose a pull request (PR). But only the core-maintainers of the
project can merge PR.

### Minimal requirements for a PR

The following are the minimal requirements that every PR needs to meet.

- **Pass Continuous Integration (CI)**: Every PR has to pass our CI. This
  includes compilation with a range of compilers and for a range of target
  architectures, passing the unit tests and no detected issues with static code
  analysis tools.

- **Code-Style**: Please consider the
  [Code-Style](https://github.com/open62541/open62541/wiki/Code-Style)
  recommendations when formatting your code.

- **Signed CLA**: Every contributor must sign the Contributor License Agreement
  (CLA) before we can merge his first PR. The signing can be done online. The
  link automatically appears on the page of the first PR. In addition, the CLA
  text can be accessed [here](https://cla-assistant.io/open62541/open62541).

### Commit and PR Hygiene

- **Separation of Concerns**: Small changes are much easier to review.
  Typically, small PR are merged much faster. For larger contributions, it might
  make sense to break them up into a series of PR. For example, a PR with a new
  feature should not contain other commits with only stylistic improvements to
  another portion of the code.

- **Feature Commits**: The same holds true for the individual PR as well. Every
  commit inside the PR should do one thing only. If many changes have been
  applied at the same time, `git add --patch` can be used to partially stage and
  commit changes that belong together.

- **Commit Messages**: Good commit messages help in understanding changes.
  Consider the following article with best practices for commit messages:
  https://chris.beams.io/posts/git-commit

- **Linear Commit History**: Our goal is to maintain a linear commit history
  where possible. Use the `git rebase` functionality before pushing a PR. Use
  `git rebase --interactive` to squash bugfix commits.

### Review Process

The following labels can be used for the PR title to indicate its status.

- [WIP]: The PR is work in progress and at this point simply informative.
- [Review]: The PR is ready from the developers perspective. He requests a review from a core-maintainer.

The core-maintainers are busy people. If they take especially long to react,
feel free to trigger them by additional comments in the PR thread. Again, small
PR are much faster to review.

It is the job of the developer that posts the PR to rebase the PR on the target
branch when the two diverge.

### Changes to the public API

The *public* API is the collection of header files in the /include folder.

Changes to the public API are under especially high scrutiny. Public API changes
are best discussed with the core-maintainers early on. Simply to avoid duplicate
work when changes to the proposed API become necessary.

You can create a special issue or PR just for the sake of discussing a proposed
API change. The actual implementation can follow later on.