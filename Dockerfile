FROM alpine:3.3
RUN apk add --no-cache cmake gcc musl-dev python py-lxml make && rm -rf /var/cache/apk/*
ADD . /tmp/open62541
WORKDIR /tmp/open62541/build
RUN cmake -D UA_ENABLE_AMALGAMATION=true /tmp/open62541 && make
RUN cp *.h /usr/include/ && \
    cp *.so /usr/lib && \
    cp *.a /usr/lib
