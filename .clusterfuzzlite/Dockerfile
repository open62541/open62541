FROM gcr.io/oss-fuzz-base/base-builder:v1
RUN apt-get update && apt-get install -y make cmake python3-six libssl-dev zip
COPY . $SRC/open62541
WORKDIR $SRC/open62541
RUN git submodule update --init --recursive deps/mdnsd
COPY .clusterfuzzlite/build.sh $SRC/
