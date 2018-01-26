#!/bin/bash
set -ev

xcode-select --install

brew install check
brew install valgrind
brew install graphviz
brew install python
brew install mbedtls

pip2 install --user sphinx
pip2 install --user breathe
pip2 install --user sphinx_rtd_theme
