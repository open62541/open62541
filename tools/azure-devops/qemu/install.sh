#!/usr/bin/env bash

set -e

apt-get update
apt install -y --allow-unauthenticated cmake python3-pip check libsubunit-dev wget tar git

wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.7.1.tar.gz
tar xf mbedtls-2.7.1.tar.gz
cd mbedtls-mbedtls-2.7.1
cmake -DENABLE_TESTING=Off .
make -j
make install 2>1
