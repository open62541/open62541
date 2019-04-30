#!/bin/bash
set -ev

brew install check
brew install --HEAD valgrind
brew install graphviz
brew install python
brew install mbedtls

pip2 install --user six
