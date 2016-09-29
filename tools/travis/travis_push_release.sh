#!/bin/bash
set -ev

TAGSTOSAVE=50
TAG="$(git rev-parse --short=10 HEAD)"
BRANCH="$(git log --pretty=format:"%d" --date=iso --abbrev=10 --all -1)"
DATE="$(git log --pretty=format:"%ad" --date=iso --abbrev=10 --all -1)"
COMMENT="$(git log --pretty=format:"%s" --date=iso --abbrev=10 --all -1)"

git clone --depth=5 -b gh-pages https://$GITAUTH@github.com/open62541/open62541-www
cd open62541-www

#hanndle releases
cd releases
if [ ! -e "$TAG.zip" ]; then
    #add the first line
    echo "<tr><td><a href='./$TAG.zip'>$TAG</a></td><td>$BRANCH</td><td>$DATE</td><td>$COMMENT</td></tr>" | cat - rawtable.txt > temp && mv temp rawtable.txt

    #keep top lines only
    head "-$TAGSTOSAVE" rawtable.txt > temp && mv temp rawtable.txt

    #compose the index file
    cat head.txt rawtable.txt foot.txt > index.html

    #create a zip for single-file release and copy the files
    cp ../../open62541.c .
    cp ../../open62541.h .
    zip -r "$TAG.zip" open62541.c open62541.h
    rm open62541.c
    rm open62541.h
    git add "$TAG.zip"

    echo "$TAG.zip" | cat - raw.txt > temp && mv temp raw.txt

    LINETOSTART=$((TAGSTOSAVE+1))
    #remove obsolete zips
    tail -n +"$LINETOSTART" raw.txt | xargs git rm --ignore-unmatch

    #remove obsolete zips from list
    head "-$TAGSTOSAVE" raw.txt > temp && mv temp raw.txt
fi
cd ..

git config --global user.email "open62541-travis-ci@users.noreply.github.com"
git config --global user.name "Open62541 travis-ci"
git config --global push.default simple
git commit -am "added release files and updated releases webpage by travis-ci [ci skip]"
git push https://$GITAUTH@github.com/open62541/open62541-www
cd ..
rm -rf open62541-www
