#!/bin/bash

ACTION=$1 # all or clean
BUILD_MODE=${2:-DEBUG_BUILD} # RELEASE_BUILD or DEBUG_BUILD(Default: DEBUG_BUILD)

if [ "$(uname)" == 'Darwin' ]; then
    /usr/bin/make -f makefile_macos.mak $ACTION BUILD_MODE=$BUILD_MODE
else
    echo "Your platform ($(uname -a)) is not supported."
    exit 1
fi
