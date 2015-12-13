#!/bin/bash
set -ev
brew install cmake
brew install check
brew install libxml2
brew install userspace-rcu
brew install --HEAD valgrind

sudo pip install lxml
sudo pip install sphinx
sudo pip install breathe
sudo pip install sphinx_rtd_theme
