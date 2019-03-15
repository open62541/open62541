#!/bin/bash
set -e

# This script will only be executed on branches which are either master or a release
# See travis_linux_after_success.sh

# The encrypted key is not available in pull requests from forks
if [ -z "$GITAUTH" ]; then
    exit 0
fi

git clone --depth=5 -b gh-pages https://$GITAUTH@github.com/open62541/open62541-www
cd open62541-www

target_dir="${TRAVIS_BRANCH}"

if [ "${TRAVIS_BRANCH}" = "master" ]; then
    target_dir="current"
fi

echo "=== Pushing doc for ${target_dir} ==="

rm -r -f ./doc/${target_dir} || true # ignore result
mkdir ./doc/${target_dir} || true # ignore result
cp -r ../../doc/* ./doc/${target_dir}/
cp -r ../../doc_latex/open62541.pdf ./doc/open62541-${target_dir}.pdf
git add -A ./doc
git config --global user.email "open62541-travis-ci@users.noreply.github.com"
git config --global user.name "Open62541 travis-ci"
git config --global push.default simple
git commit -am "updated generated documentation on webpage by travis-ci [ci skip]"
git push https://$GITAUTH@github.com/open62541/open62541-www

cd .. && rm -rf open62541-www
