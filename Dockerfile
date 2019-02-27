FROM alpine:3.5
RUN apk add --no-cache cmake gcc g++ musl-dev python py-pip make && rm -rf /var/cache/apk/*
ADD . /tmp/open62541
WORKDIR /tmp/open62541/build
RUN cmake -DUA_ENABLE_AMALGAMATION=true  \
          -DBUILD_SHARED_LIBS=true \
          /tmp/open62541 
RUN make -j
RUN cp *.h /usr/include/ && \
    cp bin/*.so /usr/lib
