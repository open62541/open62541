FROM alpine:3.10
RUN apk add --no-cache cmake gcc git g++ musl-dev mbedtls-dev python py-pip make && rm -rf /var/cache/apk/*
ADD . /opt/open62541

# Get all the git tags to make sure we detect the correct version with git describe
WORKDIR /opt/open62541
RUN git remote add github-upstream https://github.com/open62541/open62541.git
RUN git fetch -f --tags github-upstream
# Ignore error here. This always fails on Docker Cloud. It's fine there because the submodule is alread initialized. See also:
# https://stackoverflow.com/questions/58690455/how-to-correctly-initialize-git-submodules-in-dockerfile-for-docker-cloud
RUN git submodule update --init --recursive || true

WORKDIR /opt/open62541/build
RUN cmake -DBUILD_SHARED_LIBS=ON \
		-DCMAKE_BUILD_TYPE=Debug \
		-DUA_BUILD_EXAMPLES=ON \
		# Hardening needs to be disabled, otherwise the docker build takes too long and travis fails
		-DUA_ENABLE_HARDENING=OFF \
        -DUA_ENABLE_ENCRYPTION=ON \
        -DUA_ENABLE_SUBSCRIPTIONS=ON \
        -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON \
		-DUA_NAMESPACE_ZERO=FULL \
         /opt/open62541
RUN make -j
RUN make install
WORKDIR /opt/open62541

# Generate certificates
RUN apk add --no-cache python-dev linux-headers openssl && rm -rf /var/cache/apk/*
RUN pip install netifaces==0.10.9
RUN mkdir -p /opt/open62541/pki/created
RUN python /opt/open62541/tools/certs/create_self-signed.py /opt/open62541/pki/created


EXPOSE 4840
CMD ["/opt/open62541/build/bin/examples/server_ctt" , "/opt/open62541/pki/created/server_cert.der", "/opt/open62541/pki/created/server_key.der", "--enableUnencrypted", "--enableAnonymous"]
