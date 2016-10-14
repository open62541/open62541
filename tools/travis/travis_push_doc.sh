#!/bin/bash
set -ev

git clone --depth=5 -b gh-pages https://$GITAUTH@github.com/open62541/open62541-www
cd open62541-www

rm -r -f ./doc/current/* || true # ignore result
mkdir ./doc/current
cp -r ../../doc/* ./doc/current/
git add -A ./doc/current
git config --global user.email "open62541-travis-ci@users.noreply.github.com"
git config --global user.name "Open62541 travis-ci"
git config --global push.default simple
git commit -am "updated generated documentation on webpage by travis-ci [ci skip]"
git push https://$GITAUTH@github.com/open62541/open62541-www

cd .. && rm -rf open62541-www
