#!/bin/bash

# coverage.shで途中のコマンドが失敗したら、後続処理を続けず即座に終了させる
set -eu

if [ -z "${GLCE_DIR:-}" ] ||
   [ -z "${OS_NAME:-}" ] ||
   [ -z "${MAKE_COMMAND:-}" ] ||
   [ -z "${MAKEFILE:-}" ]; then
  echo "Required build context is missing. Run coverage through build.sh."
  exit 1
fi

cd "$GLCE_DIR" || exit 1

COV_FLAGS="-fprofile-instr-generate -fcoverage-mapping"

case "$OS_NAME" in
  Darwin)
    LLVM_PREFIX="$(brew --prefix llvm)"
    LLVM_PROFDATA="$LLVM_PREFIX/bin/llvm-profdata"
    LLVM_COV="$LLVM_PREFIX/bin/llvm-cov"
    ;;
  Linux)
    LLVM_PROFDATA="$(command -v llvm-profdata || true)"
    LLVM_COV="$(command -v llvm-cov || true)"
    ;;
  *)
    echo "Your platform ($OS_NAME) is not supported."
    exit 1
    ;;
esac

if [ ! -x "$LLVM_PROFDATA" ]; then
  echo "llvm-profdata was not found or is not executable: $LLVM_PROFDATA"
  exit 1
fi

if [ ! -x "$LLVM_COV" ]; then
  echo "llvm-cov was not found or is not executable: $LLVM_COV"
  exit 1
fi

"$MAKE_COMMAND" -f "$MAKEFILE" clean
"$MAKE_COMMAND" -f "$MAKEFILE" all BUILD_MODE=TEST_BUILD COV_FLAGS="$COV_FLAGS"

# 古いカバレッジ計測ファイルを削除してからgl_choco_engineを実行する
rm -f default.profraw default.profdata
LLVM_PROFILE_FILE="default.profraw" ./bin/gl_choco_engine

"$LLVM_PROFDATA" merge -sparse default.profraw -o default.profdata
"$LLVM_COV" show ./bin/gl_choco_engine -instr-profile=default.profdata -format=html -output-dir=cov

echo "Coverage report generated: $GLCE_DIR/cov/index.html"
echo "Open the file in a web browser to view the results."
