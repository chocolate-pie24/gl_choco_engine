# Platform System architecture

## 目的と位置づけ

`Platform System`は、アプリケーション開発者がプラットフォームを意識せず、グラフィックスアプリケーションを構築できるように、
差し替え可能で全プラットフォーム共通のインターフェイスAPIを提供するサブシステムである。

## Platform Systemコンセプト

`Platform System`は、目的を達成するために、オブジェクト指向のデザインパターンであるStrategyを適用した。
Strategyパターンは以下のような構造を取る。

![strategy](./strategy.png)

GLCEにおける各モジュールと、Strategyのオブジェクトの対応は以下のようになっている。

| Strategy Object | GLCE Module        | 役割                                                                                              |
| --------------- | ------------------ | ------------------------------------------------------------------------------------------------ |
| Context         | platform_context   | 上位層に`Platform System`が保有する機能のAPI窓口を提供する                                                |
| Interface       | platform_interface | Contextにプラットフォームごとに差し替え可能な仮想関数テーブル(プラットフォーム機能を抽象化したAPIを保持)を提供する |
| Concrete1       | platform_glfw      | InterfaceにGLFW実装版vtableと、その内部実装を提供する                                                  |
| Concrete2       | Not implemented    | 対応プラットフォームが増えた際に追加する                                                                |

その他、`Platform System`では、Strategyを支えるモジュールとして、下記を提供している。

| Module             | 役割                                                                                                                                                            |
| ------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| platform_err_utils | `Platform System`の全モジュールに対して、下位レイヤーの実行結果コードを`Platform System`実行結果コードに変換する機能と、`Platform System`実行結果コードの文字列への変換機能を提供する |
| platform_types     | `Platform System`の全モジュールに対して、共通して使用されるデータ型を提供する                                                                                             |

`Platform System`の全モジュールの依存関係と、下位モジュールへの依存関係は以下のようになっている。
ここで、`platform_context`の`platform_glfw`への依存は、仮想関数テーブル取得APIの使用のためのみに使用しており、APIの具体的な実装には依存していないことに注意。

![layer](./layer.png)

### Platform Concreteの選択(現状)

現状では`Platform System`のインスタンス生成時にプラットフォームを指定することで使用するプラットフォームを選択している。

### Platform Concreteの選択(将来)

現状の仕様では、全プラットフォームの内部実装がビルド可能であることが求められるが、実現は難しい。将来的にはビルドオプションでプラットフォームを指定する方式に移行する。

## 仕組み / 内部構造 / 使い方の流れ

`Platform Context`は、システムの内部状態を管理する構造体として、`platform_context_t`を上位層に公開(型名の公開のみで内部構造は非公開)している。
`platform_context_t`構造体インスタンスは、applicationレイヤーの内部状態管理構造体である`app_state_t`がインスタンス名称`platform_context`で保持することにする。
applicationレイヤーでは、`platform_context`インスタンスのリソース確保、リソース解放の責務も担う。

ここで、`Platform System`は、システム起動時に立ち上げられ、システム終了時まで常駐する性質を持つ。
よって、実行中のリソース解放を行わないため、`Platform System`自体のメモリリソースはLinear Allocatorを使用する。
Linear Allocatorは、`app_state_t`が保持する`linear_alloc`がサブシステム用アロケータの役割を担っているため、これを使用する。

使い方の流れは以下のようになる。

| フェイズ          | 処理内容                            | 方法                                                                                                 |
| --------------- | ---------------------------------- | --------------------------------------------------------------------------------------------------- |
| 初期化           | `platform_context`のリソース確保      | `application_create()`にて`platform_initialize()`を呼び出し                                           |
| 初期化           | 描画ウィンドウの生成                   | `application_create()`にて`platform_window_create()`を呼び出し(`renderer_frontend`作成後に移動予定)      |
| 終了時           | `platform_context`のリソース解放      | `application_destroy()`にて`platform_destroy()`を呼び出し                                             |
| 実行時(毎フレーム) | Platformレイヤーからのイベントの吸い上げ | `application_run()`にて`platform_pump_messages()`を呼び出し(毎フレーム)                                 |
| 実行時(毎フレーム) | ダブルバッファによるバッファスワップ     | `application_run()`にて`platform_swap_buffers()`を呼び出し(毎フレーム)(`renderer_frontend`作成後に移動予定) |

`platform_pump_messages()`によるイベントの吸い上げについては、[Event System Guide](../../guide/event_system/event_ja.md)を参照のこと。

## 現状の非対応項目

現状では以下には対応していない。GLCEの機能拡張に伴い、必要に応じて対応する。

- スレッドセーフなAPIの提供
- 実行時の使用プラットフォーム切り替え

## 設定方法

現状では設定項目はなし。

## 参照

対応プラットフォームを追加する際には、[Platform System Guide](../../guide/platform_system/adding_concretes_ja.md)を参照のこと。
