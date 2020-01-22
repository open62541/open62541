#!/usr/bin/env bash
set -e

#DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
#BASE_DIR="$( realpath "$DIR/../../../" )"

echo -e "== Building the Debian package =="
/usr/bin/python3 ./tools/prepare_packaging.py
echo -e "--- New debian changelog content ---"
echo "--------------------------------------"
cat ./debian/changelog
echo "--------------------------------------"
#dpkg-buildpackage -b

echo "Test file1" > ../libopen62541_dev.deb
echo "Test file2" > ../libopen62541_tools.deb

if [ $? -ne 0 ] ; then exit 1 ; fi

echo "Copying .deb files to $BUILD_ARTIFACTSTAGINGDIRECTORY"
cp ../libopen62541*.deb $BUILD_ARTIFACTSTAGINGDIRECTORY
