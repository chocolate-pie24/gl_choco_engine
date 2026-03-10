# カメラモジュールがそれなりにできたので、MVP行列を機能させる

## 準備

- renderer/renderer_resources/ui_shaderモジュール新規作成
  - [x] 既存のapplication.cからシェーダーモジュール生成コードを移す
  - ui_shader内部状態管理構造体で各ユニフォーム変数のlocationを管理する
  - ui_shaderモジュールの責務は、シェーダープログラムの読み込み、backendを使用した各種ユニフォームインデックスの取得、backendを使用したMVP行列等のユニフォーム変数の送信
- renderer_backend/renderer_backend_interface/interface_shaderのvtableにuniform_location取得,送信関数(mat4x4f_t, vec4f_t, etc...)を追加, concretesもそれに合わせて変更
- assets/shaders/にui_shader.vert、.fragを追加し作成

## MVP行列

一旦はapplication.c内に下記の処理を追加する。その後、renderer_frontendを新設し、そちらに移動する。

- 当面はモデル行列は単位行列でいく
- ウィンドウサイズ変更後にプロジェクション行列を再生成する
- MVP行列をGPU側へ送信する
- ui_shaderの頂点座標をvec2に変更

## TODO

- renderer_rslt_convert_choco_stringテスト
- renderer_rslt_convert_fs_utilsテスト
