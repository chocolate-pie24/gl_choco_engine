#!/bin/bash

# Sanitizers
# Leak checking (LSan) is performed on Linux only.
# LeakSanitizer output may not appear in the VSCode Debug Console (it may only abort).
# If a leak is detected, run from a terminal to get the full leak report and stack traces.

SAN_CFLAGS=-fsanitize="address,undefined"
SAN_CFLAGS+=" -fno-sanitize-recover=all"
SAN_CFLAGS+=" -fno-omit-frame-pointer"
SAN_CFLAGS+=" -fsanitize-address-use-after-scope"
SAN_CFLAGS+=" -O1"

SAN_LDFLAGS="-fsanitize=address,undefined"

if [ "$(uname)" == 'Darwin' ]; then
    /usr/bin/make -f makefile_macos.mak clean
    /usr/bin/make -f makefile_macos.mak all BUILD_MODE=TEST_BUILD SAN_CFLAGS="${SAN_CFLAGS}" SAN_LDFLAGS="${SAN_LDFLAGS}"

    ASAN_OPTIONS=halt_on_error=1:abort_on_error=1:detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./bin/gl_choco_engine
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
    /usr/bin/make -f makefile_linux.mak clean
    /usr/bin/make -f makefile_linux.mak all BUILD_MODE=TEST_BUILD SAN_CFLAGS="${SAN_CFLAGS}" SAN_LDFLAGS="${SAN_LDFLAGS}"

    ASAN_OPTIONS=halt_on_error=1:abort_on_error=1:detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./bin/gl_choco_engine
else
  echo "Your platform ($(uname -a)) is not supported."
  exit 1
fi
