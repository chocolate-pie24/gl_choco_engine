<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 chocolate-pie24 -->
<!-- AI Generated: ChatGPT (OpenAI) -->

# BDF Font Atlas Generator

## 1. 目的

`generate.py`は、固定幅のBDF（Bitmap Distribution Format）Fontから、GLCEで使用する静的なFont Atlas Assetを生成するOffline Toolである。

BDFに収録されたGlyph（文字のBitmap）から印字可能ASCIIを抽出し、一定の規則で一枚のFont Atlasに配置する。Font Atlasは1bit/pixelのPacked Dataと、そのデータを解釈するためのMetadataを持つC Source / Headerとして出力する。

変換をOfflineで行うことで、GLCEのRuntimeではBDFの解析、画像ファイルのLoad、Font Atlas用CPU Pixel Dataの動的確保およびRGBA8への展開を必要としない。生成済みの静的Assetをそのまま使用できる。

本READMEは、Generatorの使用方法とともに、入力BDFの要求事項および出力されるFont Atlas AssetのBoundary Contractを説明する。

## 2. 使用方法

### 2.1. 実行環境

- Python 3.10以降。
- Pythonの標準ライブラリのみを使用する。
- GLCEのBuild環境やOpenGL Contextは不要。

### 2.2. Font Assetの生成

Repository Rootから以下を実行する。

単一Fontを生成する場合:

```sh
python3 tools/bdf_font_atlas_generator/generate.py \
    assets/fonts/spleen/spleen-5x8.bdf
```

Spleenの全サイズを生成する場合:

```sh
python3 tools/bdf_font_atlas_generator/generate.py \
    assets/fonts/spleen/*.bdf
```

複数のBDFを一度に指定できる。Wildcardの展開はShellが行う。

デフォルトの出力先は以下のディレクトリである。

```text
glce/engine/systems/renderer/resources/fonts/generated/
```

`--output-dir`で出力先を変更できる。相対パスは`glce/`を基準とし、絶対パスを指定する場合も出力先は`glce/`配下でなければならない。

入力ファイル名の拡張子を除いた部分からC識別子および出力ファイル名を生成する。例えば`spleen-5x8.bdf`からは`spleen_5x8.h`と`spleen_5x8.c`が生成される。同じ出力名になる入力の組み合わせは拒否する。

生成済みファイルは手動で編集せず、入力FontまたはGeneratorを変更した場合は再生成する。既存の同名ファイルは上書きされる。

## 3. 入力BDFの要求事項

### 3.1. 対象文字

Generatorが必要とする文字は、印字可能ASCIIの32（`0x20`、Space）から126（`0x7E`、`~`）までの95文字である。

- 対象となる95文字がすべてBDFに収録されていること。
- 対象範囲内に重複した`ENCODING`が存在しないこと。
- BDFの文字数が95文字ちょうどである必要はない。
- 対象範囲外のGlyphはAtlasへ収録しない。ただし、GeneratorはそれらのGlyphも解析するため、未対応の形式や不正なBitmap等が含まれる場合は入力を拒否することがある。

BDF内におけるGlyphの記載順序は問わない。GeneratorがASCII Codeの昇順に並べ直す。

### 3.2. 固定幅Fontの要求事項

Generatorは`FONTBOUNDINGBOX`を全Glyphに共通するCellの基準とする。

| BDF Field | 用途・要求事項 |
|---|---|
| `FONTBOUNDINGBOX` | 共通CellのWidth、Heightおよび座標Offset。Width、Heightは正の値であること |
| `ENCODING` | Glyphの文字Code。Primary Encodingを使用する |
| `DWIDTH` | 対象95文字の水平方向Advanceが共通Cell Widthと一致すること。非負のEncodingを持つGlyphの垂直方向Advanceは0であること |
| `BBX` | Glyph固有のBitmap寸法とOffset。寸法は非負であり、対象95文字のBitmapは共通Cell内に収まること |
| `BITMAP` | Glyphの1bit Bitmap。行数と各行のByte数がBBXと一致し、Byte境界のPadding Bitが0であること |

すべてのGlyphの実際のBitmap寸法が同じである必要はない。固定幅とは、本Generatorでは共通Cell Widthと水平方向Advanceが一致することを意味する。Glyph固有の形状や余白は`BBX`とOffsetから共通Cellに反映する。

可変幅Font、垂直方向にAdvanceするFont、Unicode全体のAtlas生成などは対象外とする。Spleen以外のBDFについても上記条件を満たす必要があり、任意のBDFとの互換性を保証するものではない。

### 3.3. Glyphの配置とBaselineの正規化

BDFの座標はBaselineを基準とし、Y軸の正方向は上向きである。一方、生成するFont Atlasは左上原点で、Y軸の正方向は下向きとする。

`FONTBOUNDINGBOX`を`cell`、各Glyphの`BBX`を`box`とすると、共通Cell内のGlyph配置位置は以下で求める。

```text
dx = box.x_offset - cell.x_offset
dy = cell.y_offset + cell.height - (box.y_offset + box.height)
```

これにより、Glyph固有のOffsetとBaselineをOfflineで反映する。RuntimeでGlyphごとのOffsetやBaselineを再計算する必要はない。

## 4. Font Atlasの生成規則

### 4.1. Atlas Layout

Font Atlasとは、複数のGlyphを一枚の論理画像に並べたものである。現行Generatorでは次の規則を採用する。

| 項目 | 値 |
|---|---|
| First Character | 32（`0x20`） |
| Last Character | 126（`0x7E`） |
| Glyph Count | 95 |
| Atlas Columns | 16 |
| Atlas Rows | 6 |
| Glyph Order | ASCII Code昇順、左から右、上から下 |
| Origin | 左上 |

16列は入力BDFに対する要求ではなく、Generatorが決定する出力仕様である。

```text
         Column
         0      1      2      ...  15
Row 0    0x20   0x21   0x22   ...  0x2F
Row 1    0x30   0x31   0x32   ...  0x3F
Row 2    0x40   0x41   0x42   ...  0x4F
Row 3    0x50   0x51   0x52   ...  0x5F
Row 4    0x60   0x61   0x62   ...  0x6F
Row 5    0x70   0x71   0x72   ...  Empty
```

全96 Cellのうち95 Cellを使用し、最後のCellは空のままとする。SpaceのCellも収録対象である。

収録範囲内の文字Codeに対応するCellの位置は、Metadataを用いて次のように求められる。

```text
index   = codepoint - first_character
column  = index % atlas_columns
row     = index / atlas_columns  (整数除算)
cell_x  = column * cell_width
cell_y  = row * cell_height
```

### 4.2. Font Size / Logical Dimensions / Packed Dimensions

以下の3種類の寸法は、それぞれ異なる対象を表現する。

| 名称 | 意味 | 単位 |
|---|---|---|
| Font Size（Cell Dimensions） | 1文字に割り当てられる共通Cellの寸法 | Pixel × Pixel |
| Logical Dimensions | すべてのCellを配置したFont Atlasの論理画像の寸法 | Pixel × Pixel |
| Packed Dimensions | 1bit Pixel Dataの1行あたりのByte数と行数 | Byte × 行数 |

ここでのFont SizeはPoint Sizeや画面上の表示サイズではなく、BDFの`FONTBOUNDINGBOX`に基づくCellのPixel寸法を意味する。

Font Atlasの寸法は次のように導出する。`ceil()`は端数を切り上げることを表す。

```text
cell_width, cell_height = FONTBOUNDINGBOXのWidth, Height

glyph_count   = last_character - first_character + 1
atlas_rows    = ceil(glyph_count / atlas_columns)

logical_width  = cell_width * atlas_columns
logical_height = cell_height * atlas_rows

packed_width  = ceil(logical_width / 8)
packed_height = logical_height

pixel_data_size = packed_width * packed_height  (Byte)
```

#### Spleen 5x8の具体例

1文字が横5 Pixel、縦8 PixelのFontの場合、95文字を横16 Cell、縦6 Cellに配置する。

```text
Font Size
    5 x 8 Pixel / Cell
        |
        | 横16 Cell、縦6 Cellに配置
        v
Logical Dimensions
    (5 * 16) x (8 * 6)
    = 80 x 48 Pixel
        |
        | 水平方向の8 Pixelを1 Byteに格納
        v
Packed Dimensions
    ceil(80 / 8) x 48
    = 10 Byte / Row x 48 Rows

Pixel Data Size = 10 * 48 = 480 Byte
```

Packed Dataの生成によって画像の解像度が変化するわけではない。80×48 Pixelの情報を、480 Byteのデータで表現している。

#### Spleenの各サイズにおける寸法

| Font Size | Logical Dimensions (Pixel) | Packed Dimensions (Byte/Row × Rows) | Pixel Data Size (Byte) |
|---|---:|---:|---:|
| 5x8 | 80x48 | 10x48 | 480 |
| 6x12 | 96x72 | 12x72 | 864 |
| 8x16 | 128x96 | 16x96 | 1536 |
| 12x24 | 192x144 | 24x144 | 3456 |
| 16x32 | 256x192 | 32x192 | 6144 |
| 32x64 | 512x384 | 64x384 | 24576 |

## 5. Packed Pixel Dataの表現

### 5.1. Bit Encoding

Font Atlasの各論理Pixelは、1bitで表現する。

- 1: Glyphの描画対象Pixel。
- 0: Glyphの背景（Text描画では透明として扱う）。
- 水平方向に連続する8 Pixelを1 Byteに格納する。
- MSB-firstとし、左側のPixelを上位Bitに格納する。

例えば次の8 Pixelは、1 Byteの`0xB2`として表現される。

```text
Pixel x       0  1  2  3  4  5  6  7
Pixel value   1  0  1  1  0  0  1  0
Bit index     7  6  5  4  3  2  1  0

Binary        10110010
Hexadecimal   0xB2
```

PackingはGlyphごとではなく、Atlas全体の各Rowに対して行う。そのため、1 Byteに隣接する複数のGlyph CellのPixelが格納される場合がある。例えばCell Widthが5 PixelのFontでは、最初のByteに1文字目の5 Pixelと2文字目の先頭3 Pixelが入る。

### 5.2. Row-major Storage

Packed Byteは左上から右方向へ並べ、各Rowを上から下へ連続して格納する。

1行あたりのByte数（Row Stride）は`ceil(atlas_width / 8)`とする。論理Widthが8の倍数でない場合は各Rowの末尾の未使用Bitを0で埋め、次のRowは新しいByteから開始する。Row間に4 Byte Alignment等の追加Paddingは設けない。

なお、現行Generatorでは16列構成のため、整数のCell Widthから得られるAtlas Widthは常に8の倍数となる。

### 5.3. 任意の論理Pixelの取得

Atlas内の有効な論理座標`(x, y)`に対し、対応するByteのIndexとBitのIndexは以下のように求める（除算は整数除算）。

```text
row_stride = ceil(atlas_width / 8)
byte_index = y * row_stride + x / 8
bit_index  = 7 - (x % 8)

pixel = (pixels[byte_index] >> bit_index) & 1
```

この規則はFont Assetを生成する側と、RuntimeでPixel Dataを解釈する側の共通契約である。

## 6. 生成されるファイルとMetadata

例えば`spleen-5x8.bdf`の場合、次のファイルが生成される。

```text
resources/fonts/generated/
    spleen_5x8.h
    spleen_5x8.c
```

`.c`はPacked Pixel Dataと`font_atlas_asset_t`の実体を`static const`として保持する。`.h`はAssetのGetterを宣言する。

既存の共通`font_atlas_asset.h`を参照し、Generatorはこの共通Headerを生成・変更しない。

### 6.1. Metadata

| Field | 意味 | Spleen 5x8の例 |
|---|---|---:|
| `atlas_width` | Atlasの論理Width（Pixel） | 80 |
| `atlas_height` | Atlasの論理Height（Pixel） | 48 |
| `cell_width` | 共通Cell Width（Pixel） | 5 |
| `cell_height` | 共通Cell Height（Pixel） | 8 |
| `atlas_columns` | 1行あたりのCell数 | 16 |
| `first_character` | 収録する先頭文字のCode | 32 |
| `last_character` | 収録する末尾文字のCode | 126 |
| `advance_x` | 次の文字までの水平方向Advance（Pixel） | 5 |
| `line_height` | 次の行までの垂直方向Advance（Pixel） | 8 |
| `pixels` | Packed Pixel Dataへの読み取り専用Pointer | 静的配列への参照 |

現行Generatorは`advance_x = cell_width`、`line_height = cell_height`とする。

Packed DimensionsおよびPixel Data Sizeは論理寸法から導出できるため、Metadataに重複して保持しない。また、Glyphごとの位置やMetric Arrayも保持しない。

RuntimeでCell位置を計算する際には、95文字や16列というGeneratorの現行設定をHard Codingせず、AssetのMetadataを使用する。

### 6.2. Ownership / Lifetime

生成されたGetter（例えば`spleen_5x8_asset_get()`）は、静的な`const font_atlas_asset_t*`を返す。

- 戻り値は非NULLであり、プログラムの実行期間を通じて有効である。
- CallerはAssetおよびPixel DataをBorrowする。
- CallerはAssetおよびPixel Dataを書き換えたり解放したりしない。
- Asset取得時に動的Memory Allocationは行わない。

## 7. GeneratorのValidation

Generatorは、サポート対象となるBDFの契約を確認するため、主に以下のValidationを実施する。

| 対象 | 検証内容 |
|---|---|
| Font構造 | `STARTFONT`と`ENDFONT`の存在、`ENDFONT`以降の非空データ、正の`FONTBOUNDINGBOX`寸法 |
| Glyph構造 | `STARTCHAR` / `ENDCHAR`、必須の`ENCODING` / `DWIDTH` / `BBX` / `BITMAP` |
| Glyph Count | `CHARS`が記載されている場合、実際に解析したGlyph数との一致 |
| ASCII収録範囲 | ASCII 32〜126の欠落および対象Codeの重複 |
| Fixed Width | 対象Glyphの水平AdvanceとCell Widthの一致、垂直Advanceの制約 |
| Glyph Bounds | 対象GlyphのBBXが共通Cell内に収まること |
| Bitmap | 16進数表現、行数、行あたりのByte数、末尾Padding Bit |
| Atlas Size | 論理WidthとHeightの積が`MAX_ATLAS_PIXELS`（16,777,216 Pixel）以下であること |
| 出力先 | `glce/`配下への出力、出力名およびC識別子の重複 |

複数Fontを指定した場合、すべての入力BDFの解析とAtlas生成を完了してからファイルへの書き込みを開始する。BDFの解析・Atlas生成段階でエラーが発生した場合は出力を開始しない。ただし、書き込み中のI/Oエラーに対するファイル群全体のRollbackは提供しない。

正常終了時のExit Codeは0、捕捉した変換・I/Oエラー時は1である。CLI引数の誤りは`argparse`の処理に従う。

これらはGeneratorが必要とする入力条件のValidationであり、BDF仕様全体に対する汎用Validatorではない。また、GPUのTexture Size Limitを保証するものではない。

## 8. GLCE Runtimeとの関係

### 8.1. Static Font Catalog

生成されたFont AssetはGLCEへコンパイルされる。既存の`font_catalog`がFont NameおよびCell Width / Heightに対応するAssetへのBorrowed Pointerを提供する。

新しいFontを追加する場合は、生成ファイルに加えてStatic Catalogの対応Tableを更新する必要がある。GeneratorはCatalogを自動更新しない。

### 8.2. GPU Textureへの転送方針

GLCEのText Renderingでは、Packed Pixel DataをRGBA8へ展開せず、GL33 Backendで`GL_R8UI`の整数Textureとして転送する方針を採用している。

`GL_R8UI`では1 Texelが1 Byteなので、GPU Textureの寸法はFont AtlasのLogical DimensionsではなくPacked Dimensionsに対応する。

例えばSpleen 5x8の場合:

```text
Logical Font Atlas     : 80 x 48 Pixel
Packed Pixel Data      : 10 Byte / Row x 48 Rows
R8UI GPU Texture       : 10 x 48 Texel
```

GPU側では各論理Pixelに対応する整数TexelとBitの位置を求め、Fragment Shaderで該当Bitを抽出してGlyphを描画する。整数TextureのFilteringにはNearestを使用する。Pixel DataはTightly Packedであり、GL33 BackendではUpload時のRow Alignmentを1 Byteとして扱う。

Generator自体はOpenGLやGPU ResourceのLifecycleを管理せず、CPU上の静的Assetの生成のみを責務とする。GPU Textureの生成、破棄、ShaderおよびText GeometryはGLCE Runtime側の責務である。

この節は採用しているRuntime設計を示すものであり、Text Rendering全体の実装・動作確認が完了していることを意味しない。

## 9. License

`generate.py`および生成HeaderはMIT Licenseとする。生成C Sourceには入力Font由来のBitmap Dataが含まれるため、現行GeneratorはSpleenのLicenseに合わせて`BSD-2-Clause`を出力する。

Spleenの原本BDFおよびLicenseは`assets/fonts/spleen/`に配置する。生成C SourceのCopyrightおよびLicense通知とともに、原本FontのLicense条件を維持する。

Spleen以外のBDFを使用する場合は、変換可能かどうかとは別に、元FontのLicense、Copyright、再配布条件を確認し、Generatorが出力するLicense通知を必要に応じて変更すること。

---

本READMEの草案はChatGPT（OpenAI）を用いて生成した。入力要求および出力仕様は`generate.py`の実装とGLCEのFont / Text Rendering設計に基づく。
