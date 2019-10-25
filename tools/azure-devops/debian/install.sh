#!/usr/bin/env bash

set -e

sudo apt-get update
sudo apt install -y \
    check \
    cmake \
    debhelper \
    fakeroot \
    git \
    graphviz \
    latexmk \
    libsubunit-dev \
    python-sphinx \
    python3-pip \
    tar \
    texlive-fonts-recommended \
    texlive-generic-extra \
    texlive-latex-extra \
    wget

wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.7.1.tar.gz
tar xf mbedtls-2.7.1.tar.gz
cd mbedtls-mbedtls-2.7.1
cmake -DENABLE_TESTING=Off .
make -j
sudo make install

pip install --user cpp-coveralls
# Pin docutils to version smaller 0.15. Otherwise we run into https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=839299
pip install --user 'docutils<=0.14'
pip install --user sphinx_rtd_theme
pip install --user cpplint
