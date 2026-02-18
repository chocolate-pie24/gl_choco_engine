# Platform SystemへのConcreteモジュールの追加方法ガイドライン

このページでは、エンジン開発者が安全に対応プラットフォームを追加するためのガイドラインを提供する。

なお、`Platform System`の全体像については、[Platform System architecture](../../architecture/platform_system/architecture_ja.md)を参照のこと。

## 対応プラットフォーム追加ガイドライン

- `platform_types.h`の`platform_type_t`に追加プラットフォーム名称を追加
- `include/platform/platform_concretes/`以下にconcreteモジュール用ヘッダファイルを追加
- `src/platform/platform_concretes/`以下にconcreteモジュール用実装ファイルを追加し、`platform_backend_t`のフィールドを定義する。

`platform_backend_t`には上位層が下記の情報を取得できるフィールドを備えること(上位層は`platform_context`の公開APIを通じて取得する(backend構造体の直接参照はしない))。

- マウス座標(x, y)
- ウィンドウ幅、高さ
- フレームバッファサイズ(幅、高さ)
- マウスボタン押下状態
- キーボードボタン押下状態
- ウィンドウタイトル文字列(`choco_string_t`で保有する)

concreteモジュール用実装ファイルには`Platform Interface`用仮想関数テーブルの実装として、以下の機能を実装する。

| カテゴリー       | 関数名称                     | 役割                                                                                         |
| -------------- | --------------------------- | ------------------------------------------------------------------------------------------- |
| Lifecycle      | `platform_preinit`          | 内部状態管理構造体`platform_backend_t`の構造体サイズ、アライメント要件を上位層に渡す                   |
| Lifecycle      | `platform_init`             | プラットフォームAPIの初期化(必要であれば)と`platform_backend_t`の初期化を行う                        |
| Lifecycle      | `platform_destroy`          | プラットフォームAPIの終了処理(必要であれば)と`platform_backend_t`で保持しているデータのリソースを解放する |
| Window         | `platform_window_create`    | ウィンドウを生成する                                                                            |
| Window         | `platform_swap_buffers`     | ダブルバッファリングによるバッファスワップ処理を行う                                                 |
| Event pipeline | `platform_snapshot_collect` | イベントを収集する                                                                             |
| Event pipeline | `platform_snapshot_process` | 収集したイベントを処理し、上位層が処理できるデータ形式に変換する                                       |
| Event pipeline | `platform_pump_messages`    | 上位層から渡されたコールバック関数にイベントを渡す                                                   |

- `platform_context.c`の`platform_vtable_get()`に追加したプラットフォーム用vtableを追加
- `platform_context.c`の`platform_type_valid_check()`で追加したプラットフォームを有効化する
- 新規に追加したプラットフォームを使用する場合は、`application_create()`内の`platform_initialize()`でプラットフォーム種別を指定する(*1)

*1: 将来的にはビルドオプションで指定するように変更予定
