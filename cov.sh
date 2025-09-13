#!/bin/bash

LLVM_PROFILE_FILE="default.profraw" ./bin/gl_choco_engine
/opt/homebrew/opt/llvm/bin/llvm-profdata merge -sparse default.profraw -o default.profdata
/opt/homebrew/opt/llvm/bin/llvm-cov show ./bin/gl_choco_engine    -instr-profile=default.profdata -format=html -output-dir=cov
open cov/index.html
