#!/bin/bash

#
# This script is run by travis-ci and pushes generated
# Doxygen docs and single-source distributions to open62541-www
#

git clone --depth=5 -b gh-pages https://$GITAUTH@github.com/acplt/open62541-www
cd open62541-www
#handle doc
git rm -r -f doc
cp -r ../doc ./
git add -A doc/*
git config --global user.email "open62541-travis-ci@users.noreply.github.com"
git config --global user.name "Open62541 travis-ci"
git config --global push.default simple
git commit -am "updated generated documentation on webpage by travis-ci [ci skip]"
git push https://$GITAUTH@github.com/acplt/open62541-www
cd ..
rm -rf open62541-www
