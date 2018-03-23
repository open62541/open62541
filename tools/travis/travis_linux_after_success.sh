#!/bin/bash
set -ev

if [ -z ${DOCKER+x} ]; then
	# Only on non-docker builds required

	if [ "$ANALYZE" = "false" ] && [ "$CC" = "gcc" ] && [ "${TRAVIS_REPO_SLUG}" = "open62541/open62541" ]; then
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
	fi
fi
