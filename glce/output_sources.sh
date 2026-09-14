#!/bin/sh

OUTPUT="$HOME/Desktop/glce_full_review.txt"

{
    echo "===== FILE TREE ====="
    echo

    if command -v tree >/dev/null 2>&1; then
        tree .
    else
        find . -type f | sort
    fi

    echo
    echo "===== SOURCE FILES ====="

    find . -type f \( -name '*.h' -o -name '*.c' \) | sort |
    while IFS= read -r file; do
        echo
        echo "================================================================"
        echo "FILE: $file"
        echo "================================================================"
        cat "$file"
    done

} > "$OUTPUT"

echo "Generated: $OUTPUT"
