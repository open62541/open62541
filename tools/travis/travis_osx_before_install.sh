#!/bin/bash
set -e

brew install check
brew install --HEAD valgrind
brew install graphviz
brew install python
brew install mbedtls

pip2 install --user sphinx
pip2 install --user breathe
pip2 install --user sphinx_rtd_theme
