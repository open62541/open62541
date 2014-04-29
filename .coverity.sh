#!/bin/bash

#
# This sciprt is run by travis-ci and pushes the first commit
# of the day to the coverity scan service
#

COMMITS=`git log --since=today.midnight | grep commit | wc -l`

if [[ "$COMMITS" == "1" ]]; then
    #first commit a day - push changes to branch coverity_scan
    git clone -b coverity_scan https://$GITAUTH@github.com/acplt/open62541
    cd open62541
    git fetch origin
    git merge origin/master
    git config --global user.email "open62541-travis-ci@users.noreply.github.com"
    git config --global user.name "Open62541 travis-ci"
    git config --global push.default simple
    git add *
    git commit -am "push to coverity scan by travis-ci"
    git push https://$GITAUTH@github.com/acplt/open62541
    cd ..
    rm -rf open62541
else
    echo "Not the first commit of the day - no push to coverity required"
fi
