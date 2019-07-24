FROM debian:10
RUN apt-get update && apt-get install -y \
    binutils-mingw-w64-i686 \
    build-essential \
    check \
    cmake \
    debhelper \
    fakeroot \
    g++ \
    g++-multilib \
    gcc \
    gcc-mingw-w64-i686 \
    gcc-mingw-w64-x86-64 \
    git \
    graphviz \
    latexmk \
    make \
    mingw-w64 \
    musl-dev \
    pkg-config \
    python \
    python-pip \
    python-sphinx \
    python3 \
    python3-pip \
    python3-sphinx \
    texinfo \
    texlive-fonts-recommended \
    texlive-generic-extra \
    texlive-latex-extra \
    valgrind \
    wget \
    && rm -rf /var/cache/apk/*

LABEL "com.github.actions.name"="open62541 build base"
LABEL "com.github.actions.description"="Base repository for open62541 build actions"
LABEL "com.github.actions.icon"="cpu"
LABEL "com.github.actions.color"="purple"

LABEL "repository"="http://github.com/open62541/open62541"
LABEL "homepage"="http://open62541.org"
LABEL "maintainer"="Pro <pro@users.github.com>"

# Install specific tcc version
RUN mkdir tcc_install && \
    cd tcc_install && \
    wget -nv https://mirror.netcologne.de/savannah/tinycc/tcc-0.9.27.tar.bz2 && \
    tar xf tcc-0.9.27.tar.bz2 && \
    cd tcc-0.9.27 && \
    ./configure && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf tcc_install

# Install specific mbedtls version
RUN mkdir mbedtls_install && \
    cd mbedtls_install && \
    wget -nv https://github.com/ARMmbed/mbedtls/archive/mbedtls-2.7.1.tar.gz && \
    tar xf mbedtls-2.7.1.tar.gz && \
    cd mbedtls-mbedtls-2.7.1 && \
    cmake -DENABLE_TESTING=Off -DCMAKE_INSTALL_PREFIX=$LOCAL_PKG . && \
    make -j && \
    make install && \
    cd ../.. && \
    rm -rf mbedtls_install

# Install required python packages

RUN pip install --user cpp-coveralls
# Pin docutils to version smaller 0.15. Otherwise we run into https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=839299
RUN pip install --user 'docutils<=0.14'
RUN pip install --user sphinx_rtd_theme
RUN pip install --user cpplint

RUN pip3 install --user cpp-coveralls
# Pin docutils to version smaller 0.15. Otherwise we run into https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=839299
RUN pip3 install --user 'docutils<=0.14'
RUN pip3 install --user sphinx_rtd_theme
RUN pip3 install --user cpplint

RUN export PATH=/root/.local/bin:$PATH
