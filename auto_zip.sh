#!/usr/bin/env bash

# --- 設定 ------------------------------------
# zipファイルを保存するディレクトリ
ZIP_DIR="$HOME/Desktop/zips"
# --------------------------------------------

# エラーで止まるように（お好みで外してOK）
set -euo pipefail

# このスクリプトが置いてあるディレクトリを「ディレクトリA」とみなす
BASE_DIR="$(cd "$(dirname "$0")" && pwd)"

# 出力先ディレクトリ作成（既にあれば何もしない）
mkdir -p "$ZIP_DIR"

# ディレクトリAに移動
cd "$BASE_DIR"

# ディレクトリA以下の全ての「ファイル」を対象にzip化
# ただし auto_zip.sh 自身は除外する
find . -type f ! -name 'auto_zip.sh' | while IFS= read -r filepath; do
  # 先頭の "./" を削る
  relpath="${filepath#./}"

  # ファイル名部分だけ取り出す
  filename="$(basename "$relpath")"

  # 拡張子を除いた名前を取得（最後の '.' より前）
  base="${filename%.*}"

  # 拡張子がない場合はそのまま使う
  if [ "$base" = "$filename" ]; then
    zip_name="${filename}.zip"
  else
    zip_name="${base}.zip"
  fi

  zip_path="$ZIP_DIR/$zip_name"

  # -j オプションで、zip内にはディレクトリ階層を含めずに格納する
  zip -j -q "$zip_path" "$relpath"

  echo "Created: $zip_path"
done