#!/bin/sh

set -eu

echo -e "== Building the Debian package =="
dpkg-buildpackage -b
if [ $? -ne 0 ] ; then exit 1 ; fi
cp ../open62541*.deb .
# Copy for github release script
cp ../open62541*.deb ../..
