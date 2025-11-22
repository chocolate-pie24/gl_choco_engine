---
title: "step3_3: イベントポンプのリファクタリング"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook3に対応しています。

- [このステップでやること](#このステップでやること)
- [イベントポンプ処理のリファクタリング内容](#イベントポンプ処理のリファクタリング内容)
- [入力状態管理構造体の作成](#入力状態管理構造体の作成)
- [最新の状態の収集処理(platform\_snapshot\_collect)](#最新の状態の収集処理platform_snapshot_collect)
- [現在の状態と前回の状態との比較によるイベント処理(platform\_snapshot\_process)](#現在の状態と前回の状態との比較によるイベント処理platform_snapshot_process)
- [platform\_glfw\_pump\_messagesの更新](#platform_glfw_pump_messagesの更新)

## このステップでやること

前回までで、ウィンドウイベントについて、

- OSレイヤーからイベントを汲み上げ
- アプリケーションのリングキューにイベントを格納
- リングキューのイベントを処理

という一連のイベントシステムの流れができました。これから、イベントシステムをキーボードイベントやマウスイベントに拡張していくことになります。今回は、現状のイベントポンプ処理をリファクタリングすることで、今後のイベントシステムの拡張に備えます。

## イベントポンプ処理のリファクタリング内容

リファクタリング内容についてですが、現状のイベントポンプ処理はこのようになっています。

```c
static platform_result_t platform_glfw_pump_messages(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    int window_width = 0;
    int window_height = 0;
    window_event_t window_event;

    glfwPollEvents();
    const bool window_should_close = (0 != glfwWindowShouldClose(platform_backend_->window)) ? true : false;
    if(window_should_close) {
        ret = PLATFORM_WINDOW_CLOSE;
        goto cleanup;
    }
    glfwGetWindowSize(platform_backend_->window, &window_width, &window_height);

    if(window_width != platform_backend_->window_width || window_height != platform_backend_->window_height) {
        window_event.event_code = WINDOW_EVENT_RESIZE;
        window_event.window_height = window_height;
        window_event.window_width = window_width;
        window_event_callback(&window_event);
        platform_backend_->window_height = window_height;
        platform_backend_->window_width = window_width;
    }
    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

この構成は、動作上は問題はないのですが、

- glfwPollEventsによるイベントの取得
- glfwWindowShouldClose, glfwGetWindowSizeによる現在の状態の取得
- 現在の状態と、前回の状態との比較によるイベント処理

といった処理が混在し、この関数に求められる役割が多くなっています。そのため、今後キーボードイベントやマウスイベントに拡張していくと、関数がどんどん肥大化していってしまいます。なので、役割に応じて処理を分割するリファクタリングを行っていきます。

具体的には、

- 現在の状態の取得を行うplatform_snapshot_collect関数の追加
- 現在の状態と前回の状態との比較によるイベント処理としてplatform_snapshot_process関数の追加

を行っていきます。

## 入力状態管理構造体の作成

先程説明したように、現在の状態と前回の状態とを比較することでイベントを発行します。そのため、ウィンドウイベントについて言えば、現在のウィンドウサイズ情報、前回のウィンドウサイズ情報が必要となります。現状、platform_glfw_pump_messagesではローカル変数に現在の状態を代入し、変化があったら静的に保持している前回の状態値と差し替え、という処理にしています。

今回、リファクタリングによって処理を分割をしていくにあたり、取得した現在の状態値を渡す仕組みが必要となります。ポインタ引数によって渡す方法、静的に保持する方法の2つがあります。どちらを選んでも良いのですが、今回は静的に保持する方法を取ることにします。こちらの方がprv値、current値を並べて管理することになるので、構造として分かりやすいかなと思ったのが理由です。

それでは、前回値、現在値を両方管理できるように構造体を変更していきます。現状、platform_backend_tの中身はこのようになっています。

```c
struct platform_backend {
    choco_string_t* window_label;
    GLFWwindow* window;
    bool initialized_glfw;

    int window_width;
    int window_height;
};
```

この中のwindow_width, window_heightについて前回値と現在値を作っていく必要があるのですが、今後、キーボードやマウス等、イベントが増えるのは明確であるため、各変数個別に両方管理していくと変数が非常に多くなってしまいます。なので、これらの値を格納する構造体を新規に作成します。入力値情報のスナップショットを保持する構造体です。

```c
typedef struct input_snapshot {
    int window_width;   /**< ウィンドウ幅 */
    int window_height;  /**< ウィンドウ高さ */

    bool window_should_close;   /**< ウィンドウクローズイベント発生 */
} input_snapshot_t;
```

この構造体を新規に追加したことで、platform_backend_tはこのようになります。

```c
struct platform_backend {
    choco_string_t* window_label;   /**< ウィンドウラベル */
    GLFWwindow* window;             /**< GLFWウィンドウオブジェクト */
    bool initialized_glfw;          /**< GLFW初期済みフラグ */
    input_snapshot_t current;       /**< 入力状態のスナップショット(現在値) */
    input_snapshot_t prev;          /**< 入力状態のスナップショット(前回値) */
};
```

以上でデータ構造の準備ができました。この変更により、platform_glfw_window_createにちょっとだけ変更が必要です。ウィンドウサイズの初期化部分だけ変更しています。

```c
static platform_result_t platform_glfw_window_create(platform_backend_t* platform_backend_, const char* window_label_, int window_width_, int window_height_) {
    // 省略

    // https://www.glfw.org/docs/latest/group__input.html#gaa92336e173da9c8834558b54ee80563b
    glfwSetInputMode(platform_backend_->window, GLFW_STICKY_KEYS, GLFW_TRUE);  // これでエスケープキーが押されるのを捉えるのを保証する
    platform_backend_->prev.window_height = window_height_;
    platform_backend_->prev.window_width = window_width_;
    platform_backend_->current.window_height = window_height_;
    platform_backend_->current.window_width = window_width_;

    // 省略
}
```

以降はplatform_glfw_pump_messagesのリファクタリングをしていきます。

## 最新の状態の収集処理(platform_snapshot_collect)

platform_snapshot_collectは最新の状態を収集するのが役割です。この処理の終了時点ではまだイベントにはしておらず、現在の状態のスナップショットを取得することが目的です。

既存のplatform_glfw_pump_messagesから入力状態の取得部分を抜き出し、先程追加したinput_snapshot_t currentに対して値を格納するようにします。

```c
static platform_result_t platform_snapshot_collect(platform_backend_t* platform_backend_) {
    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_->window")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_collect", "platform_backend_->initialized_glfw")

    platform_backend_->current.window_should_close = (0 != glfwWindowShouldClose(platform_backend_->window)) ? true : false;

    glfwGetWindowSize(platform_backend_->window, &platform_backend_->current.window_width, &platform_backend_->current.window_height);

    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

## 現在の状態と前回の状態との比較によるイベント処理(platform_snapshot_process)

収集した情報に基づいてイベント処理を行うのがplatform_snapshot_process関数です。前回の状態と今回の状態を比較し、変化があったときだけ、イベントを発行します。

既存のplatform_glfw_pump_messagesからイベント処理の部分を抜き出します。

```c
static platform_result_t platform_snapshot_process(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "platform_backend_")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "platform_backend_->window")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "platform_backend_->initialized_glfw")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_snapshot_process", "window_event_callback")

    if(platform_backend_->current.window_should_close) {
        ret = PLATFORM_WINDOW_CLOSE;
        platform_backend_->prev = platform_backend_->current;
        goto cleanup;
    }

    // window event
    if(platform_backend_->current.window_width != platform_backend_->prev.window_width || platform_backend_->current.window_height != platform_backend_->prev.window_height) {
        window_event_t window_event;
        window_event.event_code = WINDOW_EVENT_RESIZE;
        window_event.window_height = platform_backend_->current.window_height;
        window_event.window_width = platform_backend_->current.window_width;

        window_event_callback(&window_event);
    }

    platform_backend_->prev = platform_backend_->current;
    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

## platform_glfw_pump_messagesの更新

これらの関数を追加することにより、platform_glfw_pump_messagesをシンプルにすることができます。

```c
static platform_result_t platform_glfw_pump_messages(
    platform_backend_t* platform_backend_,
    void (*window_event_callback)(const window_event_t* event_)) {

    platform_result_t ret = PLATFORM_INVALID_ARGUMENT;

    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_, PLATFORM_INVALID_ARGUMENT, "platform_glfw_pump_messages", "platform_backend_")
    CHECK_ARG_NOT_VALID_GOTO_CLEANUP(platform_backend_->initialized_glfw, PLATFORM_INVALID_ARGUMENT, "platform_glfw_pump_messages", "platform_backend_->initialized_glfw")
    CHECK_ARG_NULL_GOTO_CLEANUP(window_event_callback, PLATFORM_INVALID_ARGUMENT, "platform_glfw_pump_messages", "window_event_callback")
    CHECK_ARG_NULL_GOTO_CLEANUP(platform_backend_->window, PLATFORM_INVALID_ARGUMENT, "platform_glfw_pump_messages", "platform_backend_->window")

    // イベントの取得
    glfwPollEvents();

    ret = platform_snapshot_collect(platform_backend_);
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_pump_messages(%s) - Failed to collect snapshot.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = platform_snapshot_process(platform_backend_, window_event_callback);
    if(PLATFORM_WINDOW_CLOSE == ret) {
        goto cleanup;
    }
    if(PLATFORM_SUCCESS != ret) {
        ERROR_MESSAGE("platform_glfw_pump_messages(%s) - Failed to process snapshot.", rslt_to_str(ret));
        goto cleanup;
    }
    ret = PLATFORM_SUCCESS;

cleanup:
    return ret;
}
```

以上の変更により、今後イベントシステムが拡張されることにより、platform_snapshot_collectとplatform_snapshot_processの処理は増えていくことになりますが、情報の収集と、情報の処理と関数の責務が分割されたため、シンプルに拡張していくことができるようになりました。

これでリファクタリングは完了です。次回は、これまでに作成したイベントシステムの仕組みを使用し、マウスイベントを追加していきます。
