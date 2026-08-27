# Build Stage (1/2)
FROM alpine:3.20 AS build
RUN apk add --no-cache cmake gcc git g++ musl-dev mbedtls-dev linux-headers python3 py3-pip make && rm -rf /var/cache/apk/*
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
      -DUA_ENABLE_ENCRYPTION=MBEDTLS \
      -DUA_ENABLE_SUBSCRIPTIONS=ON \
      -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
      -DUA_NAMESPACE_ZERO=FULL \
      -DUA_ENABLE_DRIVER_GDS_RECEIVER=ON \
      -DUA_ENABLE_RBAC=ON \
      /opt/open62541 && \
    make -j && \
    make install
WORKDIR /opt/open62541


# Final Stage (2/2)
FROM alpine:3.20 AS server

RUN apk add --no-cache mbedtls python3 py3-netifaces openssl
RUN --mount=src=/,dst=/artifacts,from=build \
    mkdir -p /opt/open62541/pki \
    && mkdir -p /usr/local/lib/cmake/open62541 \
    && mkdir -p /usr/local/lib/pkgconfig \
    && mkdir -p /usr/local/share/open62541 \
    && mkdir -p /usr/local/include/open62541 \
    && mkdir -p /usr/local/bin \
    && cp -r /artifacts/opt/open62541/build/bin/examples /opt/open62541 \
    && cp -r /artifacts/usr/local/lib/libopen62541* /usr/local/lib/ \
    && cp -r /artifacts/usr/local/lib/cmake/open62541/ /usr/local/lib/cmake/open62541/ \
    && cp -r /artifacts/usr/local/lib/pkgconfig/open62541.pc /usr/local/lib/pkgconfig/open62541.pc \
    && cp -r /artifacts/usr/local/share/open62541/certs /usr/local/share/open62541/certs \
    && cp -r /artifacts/usr/local/include/open62541/ /usr/local/include/open62541/ \
    && ln -s /opt/open62541/examples/client /usr/local/bin/ua_client \
    && ln -s /opt/open62541/examples/ci_server /usr/local/bin/ua_server_ctt \
    && ln -s /opt/open62541/examples/ci_server /opt/open62541/examples/server_ctt \
    && cp /artifacts/opt/open62541/tools/docker/entrypoint.sh /entrypoint.sh \
    && chmod +x /entrypoint.sh

ENV LD_LIBRARY_PATH=/usr/local/lib/
ENV UA_PKI_DIR=/opt/open62541/pki

EXPOSE 4840
ENTRYPOINT ["/entrypoint.sh"]
