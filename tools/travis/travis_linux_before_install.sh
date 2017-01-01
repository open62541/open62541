#!/bin/bash
set -ev

echo "=== Installing from external package sources ==="
wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
echo "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main" | sudo tee -a /etc/apt/sources.list
sudo add-apt-repository -y ppa:lttng/ppa
sudo apt-get update -qq
sudo apt-get install -y clang-3.9 clang-tidy-3.9
sudo apt-get install -y liburcu4 liburcu-dev

echo "=== Installing python packages ==="
pip install --user cpp-coveralls
pip install --user sphinx
pip install --user sphinx_rtd_theme

echo "=== Installed versions are ==="
clang --version
g++ --version
cppcheck --version
valgrind --version
