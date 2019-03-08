#!/bin/bash
set -ev

brew install check
brew install --HEAD valgrind
brew install graphviz
brew install python
brew install mbedtls
brew install clang-format

pip2 install --user sphinx
pip2 install --user breathe
pip2 install --user sphinx_rtd_theme
