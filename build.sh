#!/bin/bash

ACTION=$1 # all or clean
BUILD_MODE=${2:-DEBUG_BUILD} # RELEASE_BUILD or DEBUG_BUILD(Default: DEBUG_BUILD)
SAN=${3:-0} # Sanitizer(0 or 1)(Default: 0)

if [ "$(uname)" == 'Darwin' ]; then
  /usr/bin/make -f makefile_macos.mak $ACTION BUILD_MODE=$BUILD_MODE SAN=$SAN
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
  /usr/bin/make -f makefile_linux.mak $ACTION BUILD_MODE=$BUILD_MODE SAN=$SAN
else
  echo "Your platform ($(uname -a)) is not supported."
  exit 1
fi
