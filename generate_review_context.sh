#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

SOURCE_OUTPUT="$SCRIPT_DIR/glce_source_context.txt"
DESIGN_OUTPUT="$SCRIPT_DIR/glce_design_context.txt"

cd "$SCRIPT_DIR"

# =============================================================================
# Source context
# =============================================================================

{
    echo "===== GLCE SOURCE CONTEXT ====="
    echo
    echo "このファイルは、GLCEのソースコードをAIによるコードレビュー、"
    echo "調査、リファクタリング検討などに利用しやすいよう、1つのテキストファイルへ"
    echo "まとめたものです。"
    echo
    echo "以下の内容を収録しています。"
    echo
    echo "- リポジトリのディレクトリツリー"
    echo "- glce/ 以下にある *.h および *.c ファイルの内容"
    echo
    echo "各ソースファイルの内容は、以下の形式のヘッダに続けて、"
    echo "元ファイルの内容を変更せずに出力しています。"
    echo
    echo "================================================================"
    echo "SOURCE: glce/<path>"
    echo "================================================================"
    echo
    echo "このファイル自体は正本ではありません。"
    echo "正本はリポジトリ内の各ソースファイルです。"
    echo
    echo "===== FILE TREE ====="
    echo

    if command -v tree >/dev/null 2>&1; then
        tree -I 'glce_source_context.txt|glce_design_context.txt' .
    else
        find . \
            ! -name 'glce_source_context.txt' \
            ! -name 'glce_design_context.txt' \
            | sort
    fi

    echo
    echo "===== SOURCE FILES ====="

    find glce -type f \( -name '*.h' -o -name '*.c' \) | sort |
    while IFS= read -r file; do
        echo
        echo "================================================================"
        echo "SOURCE: $file"
        echo "================================================================"
        cat "$file"
    done

} > "$SOURCE_OUTPUT"

# =============================================================================
# Design context
# =============================================================================

{
    echo "===== GLCE DESIGN CONTEXT ====="
    echo
    echo "このファイルは、docs/design/*.md に配置されているGLCEの設計ドキュメントを、"
    echo "AIによる設計議論、コードレビュー、要約、質問応答などに利用しやすいよう、"
    echo "1つのテキストファイルへまとめたものです。"
    echo
    echo "各ドキュメントの内容は、以下の形式のヘッダに続けて、"
    echo "元のMarkdownファイルの内容を変更せずに出力しています。"
    echo
    echo "================================================================"
    echo "DOCUMENT: docs/design/<filename>.md"
    echo "================================================================"
    echo
    echo "ドキュメントはファイルパス順に収録されます。"
    echo
    echo "このファイル自体は正本ではありません。"
    echo "正本は docs/design/ 以下の各Markdownファイルです。"
    echo
    echo "===== DESIGN DOCUMENTS ====="

    find docs/design -maxdepth 1 -type f -name '*.md' | sort |
    while IFS= read -r file; do
        echo
        echo "================================================================"
        echo "DOCUMENT: $file"
        echo "================================================================"
        cat "$file"
    done

} > "$DESIGN_OUTPUT"

echo "Generated: $SOURCE_OUTPUT"
echo "Generated: $DESIGN_OUTPUT"
