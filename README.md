# gl_choco_engine

C言語・OpenGL・GLFWを用いてゲームエンジンを作っていきます。
このリポジトリは学習用で、以下のステップで開発を進めていきます：

- **Step 1**: STL形式の3Dモデル描画
- **Step 2**: UIテキストの描画
- **Step 3**: 最終的に Sponza base シーンを描画

## Inspired by

本プロジェクトは、[Travis Vroman 氏](https://kohiengine.com/)の
Kohi Game Engine に触発されて開始しました。
氏の作り方を参考にしつつ、本リポジトリでは OpenGL/GLFW を用いて独自に実装を進めています。

## 開発ログ

実装の進め方や学習の過程は[記事](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)と
[Book(Step1)](https://zenn.dev/chocolate_pie24/books/2d_rendering)にまとめてあります。

- 最新タグ: `v0.1.0-step1`（Step1 完了）

## ディレクトリ構成

```console
.
├── articles
│   └── c-glfw-game-engine-introduction.md
├── books
│   └── 2d_rendering
│       ├── config.yaml
│       ├── step1_0_introduction.md
│       ├── step1_1_application_base.md
│       ├── step1_2_application_layer.md
│       ├── step1_3_base_layer.md
│       ├── step1_4_core_memory_linear_allocator.md
│       ├── step1_5_core_memory_system.md
│       └── step1_6_doxygen.md
├── build.sh
├── cov.sh
├── docs
│   ├── development_log.md
│   ├── doxygen
│   │   └── groups.dox
│   ├── doxygen_config.md
│   ├── layer.md
│   └── todo.md
├── Doxyfile
├── images
│   ├── log_example.png
│   └── memory_system_report.png
├── include
│   ├── application
│   │   ├── application.h
│   │   └── platform_registry.h
│   └── engine
│       ├── base
│       │   ├── choco_macros.h
│       │   └── choco_message.h
│       ├── containers
│       │   ├── choco_string.h
│       │   └── ring_queue.h
│       ├── core
│       │   ├── event
│       │   │   ├── keyboard_event.h
│       │   │   ├── mouse_event.h
│       │   │   └── window_event.h
│       │   ├── memory
│       │   │   ├── choco_memory.h
│       │   │   └── linear_allocator.h
│       │   └── platform
│       │       └── platform_utils.h
│       ├── interfaces
│       │   └── platform_interface.h
│       └── platform
│           └── platform_glfw.h
├── LICENSE
├── makefile_linux.mak
├── makefile_macos.mak
├── README.md
├── src
│   ├── application
│   │   ├── application.c
│   │   └── platform_registry.c
│   ├── engine
│   │   ├── base
│   │   │   └── choco_message.c
│   │   ├── containers
│   │   │   ├── choco_string.c
│   │   │   └── ring_queue.c
│   │   ├── core
│   │   │   └── memory
│   │   │       ├── choco_memory.c
│   │   │       └── linear_allocator.c
│   │   └── platform
│   │       └── platform_glfw.c
│   └── entry.c
└── test
    └── include
        ├── test_choco_string.h
        ├── test_linear_allocator.h
        ├── test_memory_system.h
        └── test_ring_queue.h
```

## エンジンレイヤー構成

エンジンを構成するコンポーネントのレイヤー構成は、[docs/layer.md](docs/layer.md)に記載しています

## 実行環境の構築

### macOS

***テスト環境***

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

***コンパイラのセットアップ***

```bash
brew install llvm
echo 'export PATH="$(brew --prefix llvm)/bin:$PATH"' >> ~/.zshrc
exec $SHELL -l
```

***必要ライブラリのリセットアップ***

```bash
brew install glfw
brew install glew
```

### Linux

***テスト環境***

```bash
$ uname -a
Linux chocolate-pie24 6.14.0-33-generic #33~24.04.1-Ubuntu SMP PREEMPT_DYNAMIC Fri Sep 19 17:02:30 UTC 2 x86_64 x86_64 x86_64 GNU/Linux

$ clang --version
Ubuntu clang version 18.1.3 (1ubuntu1)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin
```

***コンパイラのセットアップ***

```bash
sudo apt install clang lldb lld
```

***必要ライブラリのセットアップ***

```bash
sudo apt install libglew-dev
sudo apt install libglfw3-dev
```

## ビルド

```bash
chmod +x ./build.sh
./build.sh all DEBUG_BUILD    # デバッグビルド
./build.sh all RELEASE_BUILD  # リリースビルド
./build.sh all TEST_BUILD     # テストビルド
./build.sh clean              # クリーン
```

## 実行

```bash
./bin/gl_choco_engine
```

## ライセンス

このプロジェクトは **MITライセンス** で公開されています。
詳細は [LICENSE](LICENSE) を参照してください。

## 作者

**chocolate-pie24**
GitHub: [https://github.com/chocolate-pie24](https://github.com/chocolate-pie24)
