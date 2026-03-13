@page arch_renderer_system_ja Renderer System Architecture(Japanese)

# Renderer System Architecture

`Renderer System`は、上位レイヤーへ描画機能を提供するシステムで、以下のサブレイヤーで構成される。

- グラフィックスAPIの差し替えが可能で、グラフィックスAPI非依存のインターフェイスAPIを提供する`Renderer Backend`
- `Renderer System`全体で使用されるモジュールを提供する`Renderer Core`
- 上位レイヤーが使用するための、レンダラー層の描画リソースモジュールを提供する`Renderer Resources`

なお、`Renderer Frontend`も追加される予定であるが、現状では未実装である。

![レイヤー構成図](layer.png)

## サブレイヤー詳細

### Renderer Backend

[Renderer Backend Architecture](renderer_backend/architecture_ja.md)

### Renderer Core

`Renderer Core`は下記のモジュールを提供する。

| モジュール名称        | 役割                                                                                                               |
| ------------------ | ------------------------------------------------------------------------------------------------------------------ |
| renderer_err_utils | `Renderer System`全体で使用され、下位モジュールの実行結果コードの変換と、実行結果コードの文字列化を提供する                       |
| renderer_memory    | `Renderer System`内でのメモリタグの誤用防止、不要な実行結果コード変換処理を減らすための`core/choco_memory`のラッパーAPIを提供する |
| renderer_types     | `Renderer System`内で使用するグラフィックスAPI非依存の型を提供する                                                         |

### Renderer Resources

`Renderer Resources`は現状では下記のモジュールを提供する。

| モジュール名称 | 役割                                                         |
| ------------ | ---------------------------------------------------------- |
| ui_shader    | UI描画用シェーダーリソースの生成・使用・関連ユニフォーム操作を提供する |
