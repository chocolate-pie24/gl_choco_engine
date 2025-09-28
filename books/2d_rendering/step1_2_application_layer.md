---
title: "Step1_2: アプリケーションレイヤーの追加"
free: true
---

※本記事は [全体イントロダクション](../../articles/introduction.md)のBook1に対応しています。

## このステップでやること

前回の記事では、アプリケーション土台作りとして、ビルドし実行できる環境を整えました。
今回は、そこから一歩進んで、今後開発していくことになる様々な機能を起動、実行していくための土台を作成していきます。

## エントリーポイントの変更

前回、アプリケーションのエントリーポイントとして、下記のコードを書きました。

```c
#include <stdio.h>

int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    fprintf(stdout, "Build mode: RELEASE.\n");
#endif
#ifdef DEBUG_BUILD
    fprintf(stdout, "Build mode: DEBUG.\n");
#endif
#ifdef TEST_BUILD
    fprintf(stdout, "Build mode: TEST.\n");
#endif

    return 0;
}
```

巨大なアプリケーションでは、上位レイヤーをシンプルに保つ必要があります。上位が複雑になると、その下のレイヤーも管理できなくなるからです。
なので、最上位に当たるエントリーポイントには、あまり多くの機能を詰め込まず、一目で全体像が掴める構造を維持していきます。
とりあえず、下記の構造を取ることにします。

```c
int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    fprintf(stdout, "Build mode: RELEASE.\n");
#endif
#ifdef DEBUG_BUILD
    fprintf(stdout, "Build mode: DEBUG.\n");
#endif
#ifdef TEST_BUILD
    fprintf(stdout, "Build mode: TEST.\n");
#endif

    const app_err_t app_create_result = application_create();
    if(APPLICATION_SUCCESS != app_create_result) {
        goto cleanup;
    } else {
        fprintf(stdout, "Application created successfully.\n");
    }

    const app_err_t app_run_result = application_run();
    if(APPLICATION_SUCCESS != app_run_result) {
        goto cleanup;
    } else {
        fprintf(stdout, "Application executed successfully.\n");
    }

cleanup:
    application_destroy();
    return 0;
}
```

エントリーポイントの一つ下に位置するアプリケーションレイヤーの、生成処理、実行処理、終了処理を行うだけの非常にシンプルな構成ですので、
シンプルなので要点だけ記します。今後も多少の変更は追加されるかもしれませんが、大きく変更することはありません。
なお、処理に失敗した際にはgoto文によって最後のcleanupにジャンプする構造をとっています。
一般に、gotoは余り使用しない方が良いと言われていますが、この失敗した際に最後までジャンプするという使い方に限っては、使用した方が処理がシンプルなると考えており、使用しています。

## アプリケーションレイヤーの作成

エントリーポイントに追加したapplication_create、application_run、application_destroyを作っていきます。
これらはアプリケーションレイヤーに属する処理で、下記の役目を持ちます。

- エンジンを構成する各サブシステムの構築(application_create)
- アプリケーション(=ゲーム)シナリオやシーンの初期化(application_create)
- アプリケーションメインループの実行(application_run)
- アプリケーション終了処理(application_destroy)

これらの処理を格納するヘッダファイル、ソースファイルを作っていきます。ディレクトリ構造は下記のようにします。

```console
.
├── build.sh
├── include
│   └── application
│       └── application.h
├── makefile_macos.mak
├── README.md
└── src
    ├── application
    │   └── application.c
    └── entry.c
```

## application.hの作成

application.hの内容です。各処理の返り値となるエラーコードはとりあえず成功、メモリ確保失敗、実行エラーの3種類のみ置いておきます。これは今後、必要に応じて追加していきます。

```c
#ifndef GLCE_APPLICATION_APPLICATION_H
#define GLCE_APPLICATION_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APPLICATION_SUCCESS,
    APPLICATION_NO_MEMORY,
    APPLICATION_RUNTIME_ERROR,
} app_err_t;

app_err_t application_create(void);

void application_destroy(void);

app_err_t application_run(void);

#ifdef __cplusplus
}
#endif
#endif
```

## application.c: アプリケーション内部状態管理オブジェクト

次はソースファイルなのですが、ちょっと処理が多いのと、構造の説明が必要であるため、一つづつ説明していきます。
まず、application.c内で使用する構造体を定義します。

```c
#include <stddef.h> // for NULL
#include <stdio.h>  // for fprintf TODO: remove this!!
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "application/application.h"

typedef struct app_state {
    int rubbish;    // 一時的にビルドを通すためだけの変数
} app_state_t;

static app_state_t* s_app_state = NULL;
```

このapp_state_tには、今後、エンジンを構成する各サブシステムの内部状態を保管するオブジェクトを全て詰め込んでいきます。
各サブシステム自身ではなく、アプリケーション側に持たせることで、仮にサブシステムを複数実行する必要が出てきた際にも対応できるようにします。
app_state_tのインスタンスであるs_app_stateはシングルトンとしてapplication.cに静的オブジェクトとして宣言します。

## application.c: application_createの実装

application_createでは、まだ起動するサブシステムがない状態なので、app_state_tオブジェクトインスタンス用のメモリ確保と初期化のみを行います。
今後追加していく処理が失敗することを想定し、一旦、app_state_t* tmpに対して処理を行っていき、全ての処理が成功した時のみ、静的変数のs_app_stateとtmpを差し替えています。
なお、今後、様々なサブシステムを開発していきますが、それらは全て、

```c
// begin launch all systems.
// end launch all systems.
```

この間に追加されていくことになります。

```c
app_err_t application_create(void) {
    app_err_t ret = APPLICATION_RUNTIME_ERROR;
    app_state_t* tmp = NULL;

    // Preconditions
    if(NULL != s_app_state) {
        fprintf(stderr, "[ERROR](RUNTIME_ERROR): application_create - Application state is already initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    // Simulation
    tmp = (app_state_t*)malloc(sizeof(*tmp));
    if(NULL == tmp) {
        fprintf(stderr, "[ERROR](NO_MEMORY): application_create - Failed to allocate app_state memory.\n");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    // begin launch all systems.
    // end launch all systems.

    // commit
    s_app_state = tmp;

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}
```

## application.c: application_runの実装

次にapplication_runですが、現状では特に動かす処理はないので、下記のようになります。

```c
app_err_t application_run(void) {
    app_err_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        fprintf(stderr, "[ERROR](APPLICATION_RUNTIME_ERROR): application_run - Application is not initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }
    // while(APPLICATION_SUCCESS == ret) {
    // }

cleanup:
    return ret;
}
```

## application.c: application_destroyの実装

最後にアプリケーション終了処理であるdestroyです。現状では静的変数s_app_stateのメモリ解放のみを行います。

```c
void application_destroy(void) {
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    // end cleanup all systems.

    free(s_app_state);
    s_app_state = NULL;
cleanup:
    return;
}
```

以上がアプリケーションレイヤーの追加についての説明です。
次回は、アプリケーション全体から呼び出される機能を詰め込むレイヤーであるベースレイヤーを整えていきます。
