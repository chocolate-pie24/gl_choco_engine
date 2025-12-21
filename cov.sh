#!/bin/bash

LLVM_PROFILE_FILE="default.profraw" ./bin/gl_choco_engine

if [ "$(uname)" == 'Darwin' ]; then
    /opt/homebrew/opt/llvm/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
    /opt/homebrew/opt/llvm/bin/llvm-cov show ./bin/gl_choco_engine    -instr-profile=default.profdata -format=html -output-dir=cov
    open cov/index.html
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
    /usr/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
    /usr/bin/llvm-cov show ./bin/gl_choco_engine    -instr-profile=default.profdata -format=html -output-dir=cov
else
  echo "Your platform ($(uname -a)) is not supported."
  exit 1
fi
