#!/bin/bash
set -ev

IRC_NOTIFY_SUCCESS="never"
IRC_NOTIFY_FAILURE="never"

if [ ${TRAVIS_OS_NAME} == "linux" ] && [ ${CC} == "gcc" ]; then
    if [ ${TRAVIS_BRANCH} == "0.2" ] || [ ${TRAVIS_BRANCH} == "master" ]; then
        IRC_NOTIFY_SUCCESS="always"
        IRC_NOTIFY_FAILURE="always"
    fi
fi
