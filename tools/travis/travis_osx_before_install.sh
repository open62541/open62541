#!/bin/bash
set -ev

brew update
brew install check
brew install userspace-rcu
brew install valgrind
brew install graphviz

pip install --user sphinx
pip install --user breathe
pip install --user sphinx_rtd_theme
