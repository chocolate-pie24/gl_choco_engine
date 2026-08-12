#!/bin/sh

# valgrind.shで途中のコマンドが失敗したら、後続処理を続けず即座に終了させる
set -eu

if [ -z "${GLCE_DIR:-}" ] ||
   [ -z "${OS_NAME:-}" ] ||
   [ -z "${MAKE_COMMAND:-}" ] ||
   [ -z "${MAKEFILE:-}" ]; then
  echo "Required build context is missing. Run Valgrind through build.sh."
  exit 1
fi

case "$OS_NAME" in
  Linux|FreeBSD)
    ;;
  *)
    echo "Valgrind workflow is supported on Linux and FreeBSD only."
    exit 1
    ;;
esac

if ! command -v valgrind >/dev/null 2>&1; then
  echo "Valgrind was not found."
  exit 1
fi

cd "$GLCE_DIR" || exit 1

"$MAKE_COMMAND" -f "$MAKEFILE" clean
"$MAKE_COMMAND" -f "$MAKEFILE" all BUILD_MODE=DEBUG_BUILD

echo "--- running gl_choco_engine with Valgrind... ---"

valgrind --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 ./bin/gl_choco_engine

echo "--- Valgrind run completed successfully. ---"
