#!/bin/bash
set -ev

if [ $ANALYZE = "true" ]; then
    echo "=== Skipping after_success scripts in ANALYZE mode ==="
else
    echo "=== Executing after_success scripts ==="
    if [ "$CC" = "gcc" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
        if [ ${TRAVIS_PULL_REQUEST} == "false" ] && [ ${TRAVIS_BRANCH} = "0.2" ]; then
            sh ./tools/travis/travis_push_doc.sh
            sh ./tools/travis/travis_push_coverity.sh
        fi
        sh ./tools/travis/travis_push_release.sh;
    else
        echo "  Skipping push scripts since not gcc and/or ${TRAVIS_REPO_SLUG} is not the main repo"
    fi
fi
