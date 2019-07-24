FROM debian:10
RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    gcc \
    git \
    make \
    musl-dev \
    python \
    py-pip \
    && rm -rf /var/cache/apk/*

LABEL "com.github.actions.name"="open62541 build base"
LABEL "com.github.actions.description"="Base repository for open62541 build actions"
LABEL "com.github.actions.icon"="cpu"
LABEL "com.github.actions.color"="purple"

LABEL "repository"="http://github.com/open62541/open62541"
LABEL "homepage"="http://open62541.org"
LABEL "maintainer"="Pro <pro@users.github.com>"
