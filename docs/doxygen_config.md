# doxygen関連

## Doxyfile設定

モジュール構成を
docs/doxygen/groups.doxに記載するため、INPUTにdocsを含める
その場合、OUTPUTのディレクトリをINPUTから除外するので、
EXCLUDEとEXCLUDE_PATTERNSを除外

PROJECT_NAME = "gl_choco_engine"

EXTRACT_ALL = YES（最初は一括抽出、のちに厳格化）
EXTRACT_STATIC = YES（static関数も拾う）
WARN_IF_UNDOCUMENTED = YES
WARN_NO_PARAMDOC = YES

INPUT                 = include src docs/doxygen README.md
RECURSIVE             = YES
FILE_PATTERNS         = *.h *.c *.dox *.md
USE_MDFILE_AS_MAINPAGE= README.md
EXCLUDE               = docs/doxygen/html
EXCLUDE_PATTERNS      = docs/doxygen/html/**

OUTPUT_DIRECTORY      = docs/doxygen
GENERATE_HTML         = YES
HTML_OUTPUT           = html
HTML_TIMESTAMP        = NO
GENERATE_TREEVIEW     = YES
GENERATE_XML          = NO
GENERATE_LATEX        = NO
GENERATE_MAN          = NO

## groups.dox設定

上位から下位に向けて設定する。

@defgroup engine Engine
@defgroup base Base & @ingroup engine
@defgroup core Core & @ingroup engine
@defgroup core_memory Core Memory & @ingroup core
