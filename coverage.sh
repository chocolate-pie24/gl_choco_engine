#!/bin/bash

COV_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

if [ "$(uname)" == 'Darwin' ]; then
    /usr/bin/make -f makefile_macos.mak clean
    /usr/bin/make -f makefile_macos.mak all BUILD_MODE=TEST_BUILD COV_FLAGS="${COV_FLAGS}"

    LLVM_PROFILE_FILE="default.profraw" ./bin/gl_choco_engine

    /opt/homebrew/opt/llvm/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
    /opt/homebrew/opt/llvm/bin/llvm-cov show ./bin/gl_choco_engine    -instr-profile=default.profdata -format=html -output-dir=cov
    open cov/index.html
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
    /usr/bin/make -f makefile_linux.mak clean
    /usr/bin/make -f makefile_linux.mak all BUILD_MODE=TEST_BUILD COV_FLAGS="${COV_FLAGS}"

    LLVM_PROFILE_FILE="default.profraw" ./bin/gl_choco_engine

    /usr/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
    /usr/bin/llvm-cov show ./bin/gl_choco_engine    -instr-profile=default.profdata -format=html -output-dir=cov
else
  echo "Your platform ($(uname -a)) is not supported."
  exit 1
fi
