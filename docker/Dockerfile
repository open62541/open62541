# Build Stage (1/2)
FROM alpine:3.13.6 AS build
RUN apk add --no-cache cmake gcc git g++ musl-dev mbedtls-dev python3 py3-pip make && rm -rf /var/cache/apk/*
COPY . /opt/open62541

# Get all the git tags to make sure we detect the correct version with git describe
WORKDIR /opt/open62541
RUN git remote add github-upstream https://github.com/open62541/open62541.git && \
    git fetch -f --tags github-upstream
# Ignore error here. This always fails on Docker Cloud. It's fine there because the submodule is alread initialized. See also:
# https://stackoverflow.com/questions/58690455/how-to-correctly-initialize-git-submodules-in-dockerfile-for-docker-cloud
RUN git submodule update --init --recursive || true

WORKDIR /opt/open62541/build
RUN cmake -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_BUILD_TYPE=Release \
      -DUA_BUILD_EXAMPLES=ON \
      # Hardening needs to be disabled, otherwise the docker build takes too long and travis fails
      -DUA_ENABLE_HARDENING=OFF \
      -DUA_ENABLE_ENCRYPTION=MBEDTLS \
      -DUA_ENABLE_SUBSCRIPTIONS=ON \
      -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
      -DUA_NAMESPACE_ZERO=FULL \
      /opt/open62541 && \
    make -j && \
    make install
WORKDIR /opt/open62541

# Generate certificates
RUN apk add --no-cache python3-dev linux-headers openssl && rm -rf /var/cache/apk/* && \
    pip3 install --no-cache-dir netifaces==0.10.9 && \
    mkdir -p /opt/open62541/pki/created && \
    python3 /opt/open62541/tools/certs/create_self-signed.py /opt/open62541/pki/created


# Final Stage (2/2)
FROM alpine:3.13.6 AS server

RUN apk add --no-cache mbedtls
RUN --mount=src=/,dst=/artifacts,from=build \
    mkdir -p /opt/open62541/pki \
    && mkdir -p /usr/local/lib64/cmake/open62541 \
    && mkdir -p /usr/local/lib64/pkgconfig \
    && mkdir -p /usr/local/share/open62541/tools \
    && mkdir -p /usr/local/include/open62541 \
    && mkdir -p /usr/local/bin \
    && cp -r /artifacts/opt/open62541/pki/created /opt/open62541/pki/created \
    && cp -r /artifacts/opt/open62541/build/bin/examples /opt/open62541 \
    && cp -r /artifacts/usr/local/lib64/libopen62541* /usr/local/lib64/ \
    && cp -r /artifacts/usr/local/lib64/cmake/open62541/ /usr/local/lib64/cmake/open62541/ \
    && cp -r /artifacts/usr/local/lib64/pkgconfig/open62541.pc /usr/local/lib64/pkgconfig/open62541.pc \
    && cp -r /artifacts/usr/local/share/open62541/tools/certs /usr/local/share/open62541/tools/certs \
    && cp -r /artifacts/usr/local/include/open62541/ /usr/local/include/open62541/ \
    && cp /artifacts/usr/local/include/ms_stdint.h /usr/local/include/ms_stdint.h \
    && cp /artifacts/usr/local/include/ziptree.h /usr/local/include/ziptree.h \
    && cp /artifacts/usr/local/include/aa_tree.h /usr/local/include/aa_tree.h \
    && cp /artifacts/usr/local/bin/ua_server_ctt.exe /usr/local/bin/ua_server_ctt.exe \
    && cp /artifacts/usr/local/bin/ua_client /usr/local/bin/ua_client

ENV LD_LIBRARY_PATH=/usr/local/lib64/

EXPOSE 4840
CMD ["/opt/open62541/examples/server_ctt" , "/opt/open62541/pki/created/server_cert.der", "/opt/open62541/pki/created/server_key.der", "--enableUnencrypted", "--enableAnonymous"]
