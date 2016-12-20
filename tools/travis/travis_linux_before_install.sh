#!/bin/bash
set -ev

echo "=== Updating the build environment in $LOCAL_PKG ==="

echo "=== Installing python packages ==="
pip install --user cpp-coveralls
pip install --user sphinx
pip install --user sphinx_rtd_theme

wget https://launchpad.net/~lttng/+archive/ubuntu/ppa/+build/11525342/+files/liburcu-dev_0.9.x+stable+bzr1192+pack30+201612060302~ubuntu14.04.1_amd64.deb
wget https://launchpad.net/~lttng/+archive/ubuntu/ppa/+build/11525342/+files/liburcu4_0.9.x+stable+bzr1192+pack30+201612060302~ubuntu14.04.1_amd64.deb
sudo dpkg -i *.deb
rm *.deb

echo "=== Installed versions are ==="
clang --version
g++ --version
cppcheck --version
valgrind --version
