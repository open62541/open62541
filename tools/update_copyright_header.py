#!/usr/bin/env python

# coding: UTF-8
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# It is based on the idea of http://0pointer.net/blog/projects/copyright.html

import os
import re
import io

from git import *
from shutil import move

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO


# TODO add more authors! Only use lower case name
authorFullName = {
    'pro': 'Stefan Profanter, fortiss GmbH',
    'jpfr': 'Julius Pfrommer'
}

def compactYears(yearList):

    current = None
    last = None
    result = []

    for y in yearList:
        if last is None:
            current = y
            last = y
            continue

        if y == last + 1:
            last = y
            continue

        if last == current:
            result.append("%i" % last)
        else:
            result.append("%i-%i" % (current, last))

        current = y
        last = y

    if not last is None:
        if last == current:
            result.append("%i" % last)
        else:
            result.append("%i-%i" % (current, last))

    return ", ".join(result)


def insertCopyrightAuthors(file, authorsList):
    copyrightEntries = list()
    for author in authorsList:
        copyrightEntries.append(unicode("Copyright {} (C) {}").format(compactYears(author['years']), author['author']))

    copyrightAdded = False
    commentPattern = re.compile(r"(.*)\*/$")

    tmpName = file + ".new"
    tempFile = io.open(tmpName, mode="w", encoding="utf-8")
    with io.open(file, mode="r", encoding="utf-8") as f:
        for line in f:
            if copyrightAdded or not commentPattern.match(line):
                tempFile.write(line)
            else:
                tempFile.write(commentPattern.match(line).group(1) + "\n *\n")
                for e in copyrightEntries:
                    tempFile.write(unicode(" *    {}\n").format(e))
                tempFile.write(unicode(" */\n"))
                copyrightAdded = True

    os.unlink(file)
    move(tmpName, file)

def updateCopyright(repo, file):
    print("Checking file {}".format(file))

    # Build the info on how many lines every author commited every year
    authorYearMap = dict()
    for commit, lines in repo.blame('HEAD', file):
        authorName = unicode(commit.author.name)

        if not authorName in authorYearMap:
            authorYearMap[authorName] = {}

        commitYear = commit.committed_datetime.year

        if not commitYear in authorYearMap[authorName]:
            authorYearMap[authorName][commitYear] = len(lines)
        else:
            authorYearMap[authorName][commitYear] += len(lines)

    # Now create a sorted list and filter out small contributions
    authorList = list()

    for author in authorYearMap:
        authorYears = list()
        for year in authorYearMap[author]:
            if  authorYearMap[author][year] < 10:
                # ignore contributions for this year if less than 10 lines changed
                continue
            authorYears.append(year)
        if len(authorYears) == 0:
            continue
        authorYears.sort()

        if author.lower() in authorFullName:
            authorName = authorFullName[author.name.lower()]
        else:
            authorName = author


        authorList.append({
            'author': authorName,
            'years': authorYears
        })

    # Sort the authors list first by year, and then by name

    def authorCompare(item1, item2):
        if item1['years'][0] == item2['years'][0]:
            return 0 if item1['author'] == item2['author'] else -1 if item1['author'] < item2['author'] else 1
        return item1['years'][0] - item2['years'][0]

    authorListSorted = sorted(authorList, cmp=lambda x,y: authorCompare(x,y))
    insertCopyrightAuthors(file, authorListSorted)

def walkFiles(repo, folder, pattern):
    patternCompiled = re.compile(pattern)
    for root, subdirs, files in os.walk(folder):
        for f in files:
            if patternCompiled.match(f):
                fname = os.path.join(root,f)
                updateCopyright(repo, fname)

if __name__ == '__main__':
    baseDir = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir))
    repo = Repo(baseDir)
    assert not repo.bare

    dirs = ['src', 'plugins', 'include']

    for dir in dirs:
        walkFiles(repo, os.path.join(baseDir, dir), r"(.*\.c|.*\.h)$")