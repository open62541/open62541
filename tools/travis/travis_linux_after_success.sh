#!/bin/bash
set -ev

# We only want to push the docs once, so we just take the travis run where MINGW=true which is only enabled once
if [ -z ${MINGW+x} ]; then
    echo "=== Executing after_success scripts ==="
    # List branches where the doc should be pushed to the webpage
    if [ "${TRAVIS_PULL_REQUEST}" = "false" ] && (
        [ "${TRAVIS_BRANCH}" = "master" ] || [ "${TRAVIS_BRANCH}" = "0.3" ]); then
        sh ./tools/travis/travis_push_doc.sh
        sh ./tools/travis/travis_push_coverity.sh
    fi
    sh ./tools/travis/travis_push_release.sh;
else
    echo "=== Not in the main repository or not the main build; Skip release scripts ==="
    exit 0;
fi
