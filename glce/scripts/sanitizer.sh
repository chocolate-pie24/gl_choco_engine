#!/bin/bash

# sanitizer.shで途中のコマンドが失敗したら、後続処理を続けず即座に終了させる
set -eu

if [ -z "${GLCE_DIR:-}" ] ||
   [ -z "${OS_NAME:-}" ] ||
   [ -z "${MAKE_COMMAND:-}" ] ||
   [ -z "${MAKEFILE:-}" ]; then
  echo "Required build context is missing. Run sanitizer through build.sh."
  exit 1
fi

cd "$GLCE_DIR" || exit 1

# Sanitizers
# Leak checking (LSan) is performed on Linux only.
SAN_CFLAGS="-fsanitize=address,undefined"
SAN_CFLAGS+=" -fno-sanitize-recover=all"
SAN_CFLAGS+=" -fno-omit-frame-pointer"
SAN_CFLAGS+=" -fsanitize-address-use-after-scope"
SAN_CFLAGS+=" -O1"

SAN_LDFLAGS="-fsanitize=address,undefined"

case "$OS_NAME" in
  Darwin)
    DETECT_LEAKS=0
    ;;
  Linux)
    DETECT_LEAKS=1
    ;;
  *)
    echo "Your platform ($OS_NAME) is not supported."
    exit 1
    ;;
esac

"$MAKE_COMMAND" -f "$MAKEFILE" clean
"$MAKE_COMMAND" -f "$MAKEFILE" all BUILD_MODE=TEST_BUILD SAN_CFLAGS="$SAN_CFLAGS" SAN_LDFLAGS="$SAN_LDFLAGS"

echo "--- running gl_choco_engine with sanitizers... ---"

ASAN_OPTIONS="halt_on_error=1:abort_on_error=1:detect_leaks=$DETECT_LEAKS" \
UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
./bin/gl_choco_engine

echo "--- sanitizer run completed successfully. ---"
