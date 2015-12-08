#!/bin/bash
set -ev
sudo apt-get update
sudo apt-get install binutils-mingw-w64-i686 gcc-mingw-w64-i686 mingw-w64
sudo add-apt-repository ppa:kalakris/cmake -y
sudo apt-get update -qq
sudo apt-get install -qq --no-install-recommends build-essential cmake python-lxml gcc-multilib graphviz doxygen wget zip
sudo apt-get install libsubunit-dev #for check_0.10.0
wget http://ftp.de.debian.org/debian/pool/main/c/check/check_0.10.0-3_amd64.deb
sudo dpkg -i check_0.10.0-3_amd64.deb
wget https://launchpad.net/ubuntu/+source/liburcu/0.8.5-1ubuntu1/+build/6513813/+files/liburcu2_0.8.5-1ubuntu1_amd64.deb
wget https://launchpad.net/ubuntu/+source/liburcu/0.8.5-1ubuntu1/+build/6513813/+files/liburcu-dev_0.8.5-1ubuntu1_amd64.deb
sudo dpkg -i liburcu2_0.8.5-1ubuntu1_amd64.deb
sudo dpkg -i liburcu-dev_0.8.5-1ubuntu1_amd64.deb
sudo pip install cpp-coveralls
sudo pip install sphinx
sudo pip install breathe
sudo pip install sphinx_rtd_theme
