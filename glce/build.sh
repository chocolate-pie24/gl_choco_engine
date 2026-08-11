#!/bin/sh

# build.shで途中のコマンドが失敗したら、後続処理を続けず即座に終了させる
set -eu

GLCE_DIR="$(cd "$(dirname "$0")" && pwd)"

COMMAND="${1:-build}"
MODE="${2:-debug}"
OS_NAME="$(uname -s)"

case "$COMMAND" in
  build)
    COMMAND_TYPE="make"
    MAKE_TARGET="all"

    case "$MODE" in
      debug)
        BUILD_MODE="DEBUG_BUILD"
        ;;
      release)
        BUILD_MODE="RELEASE_BUILD"
        ;;
      test)
        BUILD_MODE="TEST_BUILD"
        ;;
      *)
        echo "Unknown build mode: $MODE"
        echo "Supported build modes: debug, release, test"
        exit 1
        ;;
    esac
    ;;

  clean)
    COMMAND_TYPE="make"
    MAKE_TARGET="clean"
    BUILD_MODE=""
    ;;
  coverage)
    COMMAND_TYPE="script"
    WORKFLOW_SCRIPT="$GLCE_DIR/scripts/coverage.sh"
    ;;
  sanitize)
    COMMAND_TYPE="script"
    WORKFLOW_SCRIPT="$GLCE_DIR/scripts/sanitizer.sh"
    ;;
  valgrind)
    COMMAND_TYPE="script"
    WORKFLOW_SCRIPT="$GLCE_DIR/scripts/valgrind.sh"
    ;;

  *)
    echo "Unknown command: $COMMAND"
    echo "Supported commands: build, clean, coverage, sanitize, valgrind"
    exit 1
    ;;
esac

case "$OS_NAME" in
  Darwin)
    MAKE_COMMAND="make"
    MAKEFILE="make/macos.mk"
    ;;
  Linux)
    MAKE_COMMAND="make"
    MAKEFILE="make/linux.mk"
    ;;
  *)
    echo "Your platform ($OS_NAME) is not supported."
    exit 1
    ;;
esac

case "$COMMAND_TYPE" in
  make)
    if [ "$COMMAND" = "clean" ]; then
      "$MAKE_COMMAND" -C "$GLCE_DIR" -f "$MAKEFILE" "$MAKE_TARGET"
    else
      "$MAKE_COMMAND" -C "$GLCE_DIR" -f "$MAKEFILE" "$MAKE_TARGET" BUILD_MODE="$BUILD_MODE"
    fi
    ;;

  script)
    env \
      GLCE_DIR="$GLCE_DIR" \
      OS_NAME="$OS_NAME" \
      MAKE_COMMAND="$MAKE_COMMAND" \
      MAKEFILE="$MAKEFILE" \
      "$WORKFLOW_SCRIPT"
    ;;
esac
