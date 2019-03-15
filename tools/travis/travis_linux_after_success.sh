#!/bin/bash
set -e

# We only want to push the docs once, so we just take the travis run where MINGW=true which is only enabled once
if [ "$MINGW" = "true" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
    echo "=== Executing after_success scripts ==="
    # List branches where the doc should be pushed to the webpage
    if [ "${TRAVIS_BRANCH}" = "master" ] || [ "${TRAVIS_BRANCH}" = "0.3" ]; then
        sh ./tools/travis/travis_push_doc.sh
    fi
    sh ./tools/travis/travis_push_release.sh;
else
    echo "=== Not in the main repository or not the main build; Skip release scripts ==="
    exit 0;
fi
