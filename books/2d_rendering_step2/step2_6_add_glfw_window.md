---
title: "Step2_2: GLFWを使用したウィンドウの生成"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook2に対応しています。

## このステップでやること

前回までで、win32, X-window-systemへの拡張を前提としたプラットフォームシステムを完成させました。
今回は、このBookの最後のステップで、GLFW APIを使用してウィンドウを生成していきます。

具体的なやり方としては、前回、プラットフォームシステムに以下のような仮想関数テーブルを作りました。

interfaces/platform_interface.h

```c
typedef struct platform_vtable {
    pfn_platform_backend_preinit platform_backend_preinit;
    pfn_platform_backend_init    platform_backend_init;
    pfn_platform_backend_destroy platform_backend_destroy;
} platform_vtable_t;
```

今回はこの仮想関数テーブルにplatform_backend_window_createを追加していきます。
具体的な手順は、

1. interfaces/platform_interface.hにplatform_backend_window_createを追加
2. platform_glfwにplatform_glfw_window_createを追加
3. platform_glfwのplatform_glfw_destoryでウィンドウ破棄処理を追加
4. platform_contextにplatform_window_createを追加
5. application_runtimeにウィンドウ生成処理を追加

という流れになります。以降、順を追って説明していきます。

## interfaces/platform_interface.h

pfn_platform_backend_destroyの関数ポインタ定義の下にpfn_platform_window_createの定義を追加します。

```c
typedef void (*pfn_platform_backend_destroy)(platform_backend_t* platform_backend_);

typedef platform_result_t (*pfn_platform_window_create)(
    platform_backend_t* platform_backend_,
    const char* window_label_,
    int window_width_,
    int window_height_);
```

ウィンドウの生成には、初期化用の変数として、

- ウィンドウのタイトルとなるwindow_label_
- ウィンドウの幅(単位: pixel)
- ウィンドウの高さ(単位: pixel)

を与えます。なお、platform_glfwでの内部状態管理構造体の中でウィンドウタイトルはchoco_stringモジュールを使用することになりますが、
platform_interface.hでのcontainers/choco_string.hへの依存を避けるため、const char*を渡しています。
依存関係は利便性を失わない程度に減らすことで、テストを用意にし、全体の構成をシンプルに保ちます。

関数ポインタの定義を追加したら、仮想関数テーブルにpfn_platform_window_createを追加します。

```c
typedef struct platform_vtable {
    pfn_platform_backend_preinit platform_backend_preinit;  /**< 関数ポインタ @ref pfn_platform_backend_preinit 参照 */
    pfn_platform_backend_init    platform_backend_init;     /**< 関数ポインタ @ref pfn_platform_backend_init 参照 */
    pfn_platform_backend_destroy platform_backend_destroy;  /**< 関数ポインタ @ref pfn_platform_backend_destroy 参照 */
    pfn_platform_window_create platform_window_create;      /**< 関数ポインタ @ref pfn_platform_window_create 参照 */
} platform_vtable_t;
```

以上でinterfaces/platform_interface.hの準備が完了しました。次はplatform_glfwに具体的な実装を追加していきます。

## platform_concretes/platform_glfw.c

今回から外部APIとしてGLFWを使用していきます。また、ウィンドウラベルにchoco_stringモジュールを使用するため、
ヘッダincludeに以下を追加します。

```c
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>

#include <GL/glew.h>        // <-- 追加
#include <GLFW/glfw3.h>     // <-- 追加

#include "engine/platform_concretes/platform_glfw.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/platform/platform_utils.h"

#include "engine/interfaces/platform_interface.h"

#include "engine/containers/choco_string.h" // <-- 追加
```

GLFWと一緒にglewのincludeも行っていますが、glewはOpenGLの様々な拡張機能を使うためのAPIになります。
注意点としてはglfw3.hよりも前にglew.hをincludeする必要があります。こうしないとincludeヘッダの衝突が発生するため、glfwとglewを使う際の定番の処理だと思ってください。

次に、platform_glfwモジュールの内部状態管理構造体に下記を追加します。

```c
struct platform_backend {
    choco_string_t* window_label;   // <-- 追加
    GLFWwindow* window;             // <-- 追加
    bool initialized_glfw;
};
```

GLFWwindowというのがGLFWで生成したウィンドウへのハンドルになります。
GLFWを使用して各種操作は、このウィンドウハンドルを渡すことで行います。

次がplatform_glfw_initの変更です。

```c
static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_init", "platform_backend_")

    platform_backend_->initialized_glfw = false;

    //////////////////////////////////////////
    // ここから追加
    if(GL_FALSE == glfwInit()) {
        ERROR_MESSAGE("platform_glfw_init(%s) - Failed to initialize glfw.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);                // 4x アンチエイリアス
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef PLATFORM_MACOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);    // MacOS用
#endif
    // OpenGLプロファイル - 関数のパッケージ
    // - GLFW_OPENGL_CORE_PROFILE 最新の機能が全て含まれる
    // - COMPATIBILITY 最新の機能と古い機能の両方が含まれる
    // - 今回は最新の機能のみを使用することにする
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 古いOpenGLは使用しない

    platform_backend_->window = NULL;
    platform_backend_->window_label = NULL;
    // ここまで追加
    //////////////////////////////////////////

    platform_backend_->initialized_glfw = true;

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

追加した内容は、glfwInitによるglfwの初期化処理と、GLFWの設定です。
OpenGLは古いバージョンに互換性を持たせたAPIです。ただ、バージョン2.0以降で大きな変更がありました。

古いOpenGL(glBeginを使ってプログラミングするスタイル)では固定シェーダーといってGPU側のプログラムを自分で書く必要はなかったのですが、
最近では自分でプログラミングするようになっています。このため、プログラムの作りが古いバージョンと新しいバージョンで全く異なります。

新しいバージョンのOpenGLでは古いAPIは非推奨となっています。これら古いバージョンの互換性をどうするかという設定をここでは行っています。
今回開発するゲームエンジンは、古いAPIは使わない方針でいきますので、このような設定としています。

以上でウィンドウ生成処理を作成する準備ができましたので、先ずは関数プロトタイプ宣言に以下を追加します。

```c
static void platform_glfw_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);
static platform_result_t platform_glfw_init(platform_backend_t* platform_backend_);
static void platform_glfw_destroy(platform_backend_t* platform_backend_);

// これを追加
static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_);
```

関数プロトタイプができましたので、仮想関数テーブルに関数を追加します。

```c
static const platform_vtable_t s_glfw_vtable = {
    .platform_backend_preinit = platform_glfw_preinit,
    .platform_backend_init = platform_glfw_init,
    .platform_backend_destroy = platform_glfw_destroy,
    .platform_window_create = platform_glfw_window_create,  // <-- 追加
};
```

以降はウィンドウの具体的な生成処理を作っていきます。

### platform_glfw_window_create

次にplatform_glfw_window_createの実装です。まずは全体を貼り付けます。
ちょっと長いのでエラー処理やテスト専用コードは省いてあります。

```c
static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    ret_string = choco_string_create_from_char(&platform_backend_->window_label, window_label_);

    platform_backend_->window = glfwCreateWindow(window_width_, window_height_, choco_string_c_str(platform_backend_->window_label), NULL, NULL);

    glfwMakeContextCurrent(platform_backend_->window);
    glewExperimental = true;
    if(GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to initialize GLEW.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }

    glfwSetInputMode(platform_backend_->window, GLFW_STICKY_KEYS, GLFW_TRUE);

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

まず、関数の最初で、

```c
ret_string = choco_string_create_from_char(&platform_backend_->window_label, window_label_);
```

を行いconst char*のwindow_label_をchoco_stringモジュールに変換し、格納しています。

次が実際にウィンドウを生成する処理なのですが、このようになっています。

```c
    platform_backend_->window = glfwCreateWindow(window_width_, window_height_, choco_string_c_str(platform_backend_->window_label), NULL, NULL);
```

glfwCreateWindowというのはGLFWが提供するAPIで、ウィンドウの生成を行ってくれます。
API仕様を[GLFW window API仕様](https://www.glfw.org/docs/3.3/group__window.html#ga3555a418df92ad53f917597fe2f64aeb)から引用します。

```console
Parameters
    [in]	width	The desired width, in screen coordinates, of the window. This must be greater than zero.
    [in]	height	The desired height, in screen coordinates, of the window. This must be greater than zero.
    [in]	title	The initial, UTF-8 encoded window title.
    [in]	monitor	The monitor to use for full screen mode, or NULL for windowed mode.
    [in]	share	The window whose context to share resources with, or NULL to not share resources.
```

今回はフルスクリーンモードは使用しないことと、単一のウィンドウのみを使用するため、第四引数と第五引数にはNULLを指定しています。

次が、処理対象のウィンドウをGLFWに伝える処理です。

```c
    glfwMakeContextCurrent(platform_backend_->window);
```

ここでは、引数windowに指定したハンドルのレンダリングコンテキストをカレント(処理対象)にする処理を行っています。
レンダリングコンテキストとは、描画に用いられる情報で、ウィンドウごとに保持されます。
今後行っていく様々な描画はカレントに設定したウィンドウに対して行われます。

次が、glewの初期化処理になります。これは必ずglfwMakeContextCurrentを実行した後で行うようにします。こうしないとglewInitが失敗します。

```c
    glewExperimental = true;
    if(GLEW_OK != glewInit()) {
        ERROR_MESSAGE("platform_glfw_window_create(%s) - Failed to initialize GLEW.", s_rslt_str_runtime_error);
        ret = PLATFORM_RUNTIME_ERROR;
        goto cleanup;
    }
```

glewExperimentalをtrueにすることで、より多くのOpenGL拡張関数を取得することができるようになります。
なくても構わないのですが、とりあえず入れておくのが一般的です。筆者もこの辺は詳しくないのですが、とりあえず入れる運用にしています。
この状態でglewInitを実行してglewを初期化しています。

glewの初期化の次が、キーボード押下イベントを確実に取得するための処理です。

```c
glfwSetInputMode(platform_backend_->window, GLFW_STICKY_KEYS, GLFW_TRUE);
```

これを実行しておくことで、キーボードの押下イベントの取りこぼしがなくなる効果があります。

以上でウィンドウの生成とglewの初期化、glfwの追加設定が完了しました。次はウィンドウの破棄処理を作成していきます。

### platform_glfw_destory

ウィンドウの破棄はプラットフォームシステムの終了時に行います。なので、platform_glfw_destoryに処理を追加していきます。
ウィンドウの破棄処理を追加したコードが以下になります。

```c
static void platform_glfw_destroy(platform_backend_t* platform_backend_) {
    if(NULL == platform_backend_) {
        return;
    }
    if(NULL != platform_backend_->window) {
        glfwDestroyWindow(platform_backend_->window);
        platform_backend_->window = NULL;
    }
    if(platform_backend_->initialized_glfw) {
        glfwTerminate();
    }
    choco_string_destroy(&platform_backend_->window_label);
    platform_backend_->window = NULL;
    platform_backend_->initialized_glfw = false;
}
```

glfwDestroyWindowがウィンドウを破棄するAPIになります。ウィンドウを破棄したらglfwTerminateを呼び出してglfwを終了します。
最後にwindow_labelの文字列リソースを破棄して終了となります。なお、glewについては初期化は行いましたが、terminate処理は不要です。

以上でウィンドウの生成と破棄の具体的な処理ができました。次からはcontextモジュールにウィンドウの生成と破棄を追加することで、アプリケーションからの呼び出しを可能にしていきます。

## platform_context/platform_context.h(.c)

Contextの変更はシンプルで、backendのvtableから関数を呼び出すだけです。
先ずはヘッダに以下を追加します。

include/engine/platform_context/platform_context.h

```c
platform_result_t platform_window_create(platform_context_t* platform_context_, const char* window_label_, int window_width_, int window_height_);
```

実装はこのようになります。

src/engine/platform_context/platform_context.c

```c
platform_result_t platform_window_create(platform_context_t* platform_context_, const char* window_label_, int window_width_, int window_height_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->vtable, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->vtable")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_context_->backend, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "platform_context_->backend")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_label_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_label_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_width_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_width_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(0 != window_height_, PLATFORM_INVALID_ARGUMENT, "platform_window_create", "window_height_")

    ret = platform_context_->vtable->platform_window_create(platform_context_->backend, window_label_, window_width_, window_height_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_window_create(%s) - Failed to create window.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = PLATFORM_SUCCESS;
cleanup:
    return ret;
}
```

引数のエラーチェックが多いため長くなっていますが、実際に行う処理としては、platform_window_createを呼び出すだけです。

これでInterface, Concrete, Contextの実装が完了しましたので、最後にapplicationモジュールへウィンドウ生成処理を追加していきます。

## application.c

### 内部状態管理構造体へのメンバの追加

ウィンドウを生成する処理では引数にウィンドウの幅と高さを渡しました。当然、ウィンドウはサイズが変化します。
ウィンドウサイズが変化したら当然、画面に描画する内容も変更する必要があります。その際、以下のようなデータの流れで描画内容を変更することになります。

1. プラットフォームシステムで変化を取得
2. サイズ変化をアプリケーション側に通知
3. アプリケーションからレンダリングシステムにサイズ変化を通知

このような仕組みを取る予定なので、アプリケーション側にもウィンドウサイズを持たせるようにします。
app_state_tを以下のように変更します。

```c
typedef struct app_state {
    // application status
    int window_width;           /**< ウィンドウ幅 */
    int window_height;          /**< ウィンドウ高さ */

    // core/memory/linear_allocator
    size_t linear_alloc_mem_req;    /**< リニアアロケータオブジェクトに必要なメモリ量 */
    size_t linear_alloc_align_req;  /**< リニアアロケータオブジェクトが要求するメモリアライメント */
    size_t linear_alloc_pool_size;  /**< リニアアロケータオブジェクトが使用するメモリプールのサイズ */
    void* linear_alloc_pool;        /**< リニアアロケータオブジェクトが使用するメモリプールのアドレス */
    linear_alloc_t* linear_alloc;   /**< リニアアロケータオブジェクト */

    // platform/platform_context
    platform_context_t* platform_context; /**< プラットフォームstrategyパターンへの窓口としてのコンテキストオブジェクト */
} app_state_t;
```

window_widthとwindow_heightを追加してあります。

### application_create

次がapplication_createでのウィンドウ生成処理を追加していきます。追加後のコードが以下のようになります。
長いのでエラー処理やエラー処理関連変数の定義は省いてあります。

```c
application_result_t application_create(void) {
    app_state_t* tmp = NULL;

    // Preconditions
    ret_mem_sys = memory_system_create();

    // begin Simulation
    ret_mem_sys = memory_system_allocate(sizeof(*tmp), MEMORY_TAG_SYSTEM, (void**)&tmp);
    memset(tmp, 0, sizeof(*tmp));

    INFO_MESSAGE("Initializing linear allocator...");
    tmp->linear_alloc = NULL;
    linear_allocator_preinit(&tmp->linear_alloc_mem_req, &tmp->linear_alloc_align_req);
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM, (void**)&tmp->linear_alloc);

    tmp->linear_alloc_pool_size = 1 * KIB;
    ret_mem_sys = memory_system_allocate(tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM, &tmp->linear_alloc_pool);

    ret_linear_alloc = linear_allocator_init(tmp->linear_alloc, tmp->linear_alloc_pool_size, tmp->linear_alloc_pool);
    INFO_MESSAGE("linear_allocator initialized successfully.");

    INFO_MESSAGE("Initializing platform state...");
    ret_platform = platform_initialize(tmp->linear_alloc, PLATFORM_USE_GLFW, &tmp->platform_context);
    INFO_MESSAGE("platform_backend initialized successfully.");

    //////////////////////////////////////////////////
    // ここから追加
    tmp->window_width = 1024;
    tmp->window_height = 768;
    ret_platform = platform_window_create(tmp->platform_context, "test_window", 1024, 768);
    // ここまで追加
    //////////////////////////////////////////////////

    // commit
    s_app_state = tmp;
    INFO_MESSAGE("Application created successfully.");
    memory_system_report();
    ret = APPLICATION_SUCCESS;

cleanup:
    if(APPLICATION_SUCCESS != ret) {
        if(NULL != tmp) {
            if(NULL != tmp->platform_context) {
                platform_destroy(tmp->platform_context);
            }
            if(NULL != tmp->linear_alloc_pool) {
                memory_system_free(tmp->linear_alloc_pool, tmp->linear_alloc_pool_size, MEMORY_TAG_SYSTEM);
            }
            if(NULL != tmp->linear_alloc) {
                memory_system_free(tmp->linear_alloc, tmp->linear_alloc_mem_req, MEMORY_TAG_SYSTEM);
            }
            memory_system_free(tmp, sizeof(*tmp), MEMORY_TAG_SYSTEM);
            tmp = NULL;
        }
        memory_system_destroy();
    }

    return ret;
}
```

なお、ウィンドウの生成処理は本来、レンダリングシステムが担当する処理になります。
ただ、現状ではまだレンダリングシステムがありませんので、仮のコードとしてapplication_createで実行するようにしています。
レンダリングシステムの作成時に移動することになります。

なお、ウィンドウの破棄については、既に作成してあるplatform_destroyで行われるため、application_destroyの変更は不要です。

### application_run

最後に、このままだとウィンドウを生成し、即、終了となるため、アプリケーションが終了しないよう、無限ループを追加します。

```c
application_result_t application_run(void) {
    application_result_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", rslt_to_str(ret));
        goto cleanup;
    }
    while(1) {
    }
cleanup:
    return ret;
}
```

本来であればwhileループの中にsleepを入れるべきですが、sleep時間は目標FPSとループ内の計算時間をもとに決定する必要があり、
それらの処理はまだないため当面はこのままで活きます。

この状態でビルド、実行し、ウィンドウが出ればOKです。なお、ウィンドウを閉じる処理はまだ追加していないため、Ctrl-C等で強制的に終了してください。

以上でBook2(2d-rendering-step2)が完成となります。今回でようやくグラフィックアプリケーションの描画の土台となるウィンドウが生成できました。
次回は、今回作成したシステムをさらに拡張子、キーボード、マウス等のイベント処理を追加していきます。
なお、イベント処理についてはリポジトリでは既に実装済であるため、次回は今回よりは早いリリースができるかと思います。

// TODO: リポジトリTag
// TODO: レビュー方法 mainブランチをまず伝えて、その後にこのブランチ、差分と記事は一致しているか？不足はないか？

ここまで読んでいただきありがとうございました。
