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
    python3-sphinx \
    python3-pip \
    tar \
    texlive-fonts-recommended \
    texlive-extra-utils \
    texlive-latex-extra \
    wget

wget https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.28.2.tar.gz
tar xf mbedtls-2.28.2.tar.gz
cd mbedtls-mbedtls-2.28.2
cmake -DENABLE_TESTING=Off .
make -j
sudo make install

# Pin docutils to version smaller 0.15. Otherwise we run into https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=839299
pip install --user 'docutils<=0.14'
pip install --user sphinx_rtd_theme
pip install --user cpplint
