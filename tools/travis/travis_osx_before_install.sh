#!/bin/bash
set -ev

brew install check
brew install userspace-rcu
brew install valgrind
brew install graphviz
brew install python

pip2 install --user sphinx
pip2 install --user breathe
pip2 install --user sphinx_rtd_theme
