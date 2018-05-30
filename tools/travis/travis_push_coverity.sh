#!/bin/bash
set -ev

# This script is run by travis-ci and pushes the first commit
# of the day to the coverity scan service

echo "=== Pushing ${TRAVIS_BRANCH} to coverity_scan branch ==="

COMMITS=`git log --oneline --since=today.midnight | wc -l`
if [ "$COMMITS" -le 1 ]; then
    git push https://$GITAUTH@github.com/open62541/open62541 master:coverity_scan
else
    echo "Not the first commit of the day - no push to coverity required"
fi
