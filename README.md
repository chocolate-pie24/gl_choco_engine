# gl_choco_engine

C言語・OpenGL・GLFWを用いてゲームエンジンを作っていきます。
このリポジトリは学習用で、以下のステップで開発を進めていきます：

- **Step 1**: STL形式の3Dモデル描画
  - **Step 1_1**: 実行基盤とメモリ管理レイヤの初期化
  - **Step 1_2**: 描画ウィンドウの生成
  - **Step 1_3**: 三角形の描画
  - **Step 1_4**: イベントシステムの構築
  - **Step 1_5**: カメラの導入
  - **Step 1_6**: 立方体の描画とライティング
  - **Step 1_7**: コンテナオブジェクトの導入
  - **Step 1_8**: STLローダーの導入
- **Step 2**: UIテキストの描画
- **Step 3**: 最終的に Sponza base シーンを描画

## Inspired by

本プロジェクトは、[Travis Vroman 氏](https://kohiengine.com/)の
Kohi Game Engine に触発されて開始しました。
氏の作り方を参考にしつつ、本リポジトリでは OpenGL/GLFW を用いて独自に実装を進めています。

## 開発ログ

実装の進め方や学習の過程は [docs/development_log.md](docs/development_log.md) （Development Log）に記録しています。

## ディレクトリ構成

```console
.
├── build.sh
├── cov.sh
├── docs
│   ├── development_log.md
│   ├── layer.md
│   └── todo.md
├── include
│   ├── application
│   │   └── application.h
│   └── engine
│       ├── base
│       │   ├── choco_macros.h
│       │   └── choco_message.h
│       └── core
│           └── memory
│               ├── choco_memory.h
│               └── linear_allocator.h
├── LICENSE
├── makefile_macos.mak
├── README.md
├── src
│   ├── application
│   │   └── application.c
│   ├── engine
│   │   ├── base
│   │   │   └── choco_message.c
│   │   └── core
│   │       └── memory
│   │           ├── choco_memory.c
│   │           └── linear_allocator.c
│   └── entry.c
└── test
    └── include
        ├── test_linear_allocator.h
        └── test_memory_system.h
```

## エンジンレイヤー構成

エンジンを構成するコンポーネントのレイヤー構成は、[docs/layer.md](docs/layer.md)に記載しています

## ビルド

### 必要環境

現在、下記の環境で動作確認を行っています。
gcc、Linux環境での動作確認は今後行なっていきます。

```bash
% sw_vers
ProductName:		macOS
ProductVersion:		15.5
BuildVersion:		24F74

% /opt/homebrew/opt/llvm/bin/clang --version
Homebrew clang version 20.1.8
Target: arm64-apple-darwin24.5.0
Thread model: posix
InstalledDir: /opt/homebrew/Cellar/llvm/20.1.8/bin
Configuration file: /opt/homebrew/etc/clang/arm64-apple-darwin24.cfg
```

### ビルド

```bash
chmod +x ./build.sh
./build.sh all DEBUG_BUILD    # デバッグビルド
./build.sh all RELEASE_BUILD  # リリースビルド
./build.sh clean              # クリーン
```

### 実行

```bash
./bin/gl_choco_engine
```

## ライセンス

このプロジェクトは **MITライセンス** で公開されています。
詳細は [LICENSE](LICENSE) を参照してください。

## 作者

**chocolate-pie24**
GitHub: [https://github.com/chocolate-pie24](https://github.com/chocolate-pie24)
