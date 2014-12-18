#!/bin/bash

#
# This sciprt is run by travis-ci and pushes generated
# Doxygen docs to open62541-www
#

COMMITS=`git log --since=today.midnight --oneline | wc -l`

if [[ "$COMMITS" -le "1" ]]; then
   git clone --depth=50 -b gh-pages https://$GITAUTH@github.com/acplt/open62541-www
   cd open62541-www
   git rm -rf doc
   cp -r ../build/doc ./
   git add -A doc/*
   git config --global user.email "open62541-travis-ci@users.noreply.github.com"
   git config --global user.name "Open62541 travis-ci"
   git config --global push.default simple
   git commit -am "doxygen updated by travis-ci"
   git push https://$GITAUTH@github.com/acplt/open62541-www
   cd ..
   rm -rf open62541-www
else
    echo "Not the first commit of the day - no doxygen update required"
fi
