---
title: "step4_2: 最小限のコードで三角形を出す"
free: true
---

※本記事は [全体イントロダクション](https://zenn.dev/chocolate_pie24/articles/c-glfw-game-engine-introduction)のBook4に対応しています。

実装コードについては、リポジトリのタグv0.1.0-step4を参照してください。

# 最小限のコードで三角形を出す

このステップでは、Rendererレイヤーの構成は一旦置いておいて、最小限のコードで三角形を描画することを目指します。

そのために、既存の ***application_run()*** に対してダイレクトに描画処理を書いていきます。今回追加する処理は、ほぼ全てが外部モジュールとして外に出されることになり、将来的にはなくなる処理です。ただ、実行結果を確認しながらモジュール追加をしていくことが可能となり、結果としては効率が良い開発が行えます。

なお、今回使用するコードは、[OpenGL Tutorial](https://www.opengl-tutorial.org/jp/)のコードを使用しています。

## シェーダーオブジェクト / シェーダープログラムの作成処理の追加

GL CHOCO ENGINEで使用するOpenGLは当面3.3です。このため、GPU側のシェーダープログラムを自前で用意する必要があります。本来はassetsディレクトリ内にシェーダープログラムを配置し、シェーダーソースコードを読み込むのですが、今回は最小限の労力で三角形を表示したいため、シェーダーソースを関数内に埋め込みしてしまいます。

シェーダーには複数の種類がありますが、当面は、

- 頂点情報を処理し、ピクセル座標系に変換するバーテックスシェーダー
- 処理された頂点に色をつけるフラグメントシェーダー

のみを使用します。ここで、本プロジェクトで使用する用語の定義について書いておきます。

| 用語                | 意味                                                   |
| ------------------ | ------------------------------------------------------ |
| シェーダーソース      | シェーダーのソースコード                                   |
| シェーダーオブジェクト | シェーダーソースをコンパイルして生成されたもの                 |
| シェーダープログラム   | 複数のシェーダーオブジェクトをリンクし、生成された描画プログラム |

これらシェーダーを扱う関数を ***application.c*** に追加しました。

| 関数名          | 役割                                                      |
| -------------- | -------------------------------------------------------- |
| shader_create  | 引数で与えられたシェーダーソースをコンパイルする                 |
| program_create | シェーダーソースの記述、コンパイル、リンクを行う                |

Book4では詳細な実装の解説を行わないのですが、このステップについては、今後、最小限のコードで三角形を出したい時にコピペできるよう、実装を載せておきます。

```c
static bool shader_create(const char* shader_source_, GLenum shader_type_, GLuint* shader_id_) {
    bool ret = false;
    GLint result = GL_FALSE;
    int info_log_length = 0;

    *shader_id_ = glCreateShader(shader_type_);

    // 頂点シェーダをコンパイル
    glShaderSource(*shader_id_, 1, &shader_source_ , NULL);
    glCompileShader(*shader_id_);

    // 頂点シェーダをチェック
    glGetShaderiv(*shader_id_, GL_COMPILE_STATUS, &result);   // コンパイル結果正常でresult = GL_TRUE
    glGetShaderiv(*shader_id_, GL_INFO_LOG_LENGTH, &info_log_length); // コンパイル結果正常でinfo_log_length = 0
    if(0 != info_log_length && GL_TRUE != result) {
        char* err_mes = NULL;
        memory_system_result_t result_mem = memory_system_allocate(info_log_length, MEMORY_TAG_STRING, (void**)&err_mes);
        if(MEMORY_SYSTEM_SUCCESS != result_mem) {
            ERROR_MESSAGE("shader_create - Failed to allocate log memory.");
            ret = false;
            goto cleanup;
        }

        glGetShaderInfoLog(*shader_id_, info_log_length, NULL, err_mes);
        if(GL_VERTEX_SHADER == shader_type_) {
            INFO_MESSAGE("shader_create(vertex shader) compile log: '%s'", err_mes);
        } else if(GL_FRAGMENT_SHADER == shader_type_) {
            INFO_MESSAGE("shader_create(fragment shader) compile log: '%s'", err_mes);
        } else {
            INFO_MESSAGE("shader_create(undefined shader type) compile log: '%s'", err_mes);
        }
        memory_system_free(err_mes, info_log_length, MEMORY_TAG_STRING);
        err_mes = NULL;
        ret = false;
        goto cleanup;
    } else if(0 != info_log_length) {
        ERROR_MESSAGE("shader_create - Unknown error.");
        ret = false;
        goto cleanup;
    } else if(GL_TRUE != result) {
        ERROR_MESSAGE("shader_create - Unknown error.");
        ret = false;
        goto cleanup;
    }
    ret = true;
cleanup:
    return ret;
}

static bool program_create(void) {
    bool ret = false;
    GLint result = GL_FALSE;
    GLuint vertex_shader_id = 0;
    GLuint fragment_shader_id = 0;
    int info_log_length = 0;

    static const char* vertex_shader_source =
        "#version 330 core \n"
        "layout(location = 0) in vec3 vertexPosition_modelspace; \n"
        "void main(){ \n"
        "    gl_Position.xyz = vertexPosition_modelspace; \n"
        "    gl_Position.w = 1.0; \n"
        "} \n";
    if(!shader_create(vertex_shader_source, GL_VERTEX_SHADER, &vertex_shader_id)) {
        ERROR_MESSAGE("Failed to create vertex shader.");
        ret = false;
        goto cleanup;
    }

    static const char* fragment_shader_source =
        "#version 330 core \n"
        "out vec3 color; \n"
        "void main(){ \n"
        "    color = vec3(1,0,0);\n"
        "} \n";
    if(!shader_create(fragment_shader_source, GL_FRAGMENT_SHADER, &fragment_shader_id)) {
        ERROR_MESSAGE("Failed to create vertex shader.");
        ret = false;
        goto cleanup;
    }

    // プログラムをリンクします。
    s_app_state->program_id = glCreateProgram();
    glAttachShader(s_app_state->program_id, vertex_shader_id);
    glAttachShader(s_app_state->program_id, fragment_shader_id);
    glLinkProgram(s_app_state->program_id);

    // プログラムをチェックします。
    glGetProgramiv(s_app_state->program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(s_app_state->program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(0 != info_log_length && GL_TRUE != result) {
        char* err_mes = NULL;
        memory_system_result_t result_mem = memory_system_allocate(info_log_length, MEMORY_TAG_STRING, (void**)&err_mes);
        if(MEMORY_SYSTEM_SUCCESS != result_mem) {
            ERROR_MESSAGE("program_create - Failed to allocate log memory.");
            ret = false;
            goto cleanup;
        }
        glGetProgramInfoLog(s_app_state->program_id, info_log_length, NULL, err_mes);
        INFO_MESSAGE("program_create compile log: '%s'", err_mes);
        memory_system_free(err_mes, info_log_length, MEMORY_TAG_STRING);
        err_mes = NULL;
        ret = false;
        goto cleanup;
    } else if(0 != info_log_length) {
        ERROR_MESSAGE("program_create - Unknown error.");
        ret = false;
        goto cleanup;
    } else if(GL_TRUE != result) {
        ERROR_MESSAGE("program_create - Unknown error.");
        ret = false;
        goto cleanup;
    }

    // 既にシェーダープログラムに組み込まれたので削除
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    ret = true;
cleanup:
    return ret;
}
```

## application_runへの描画処理の追加

追加したシェーダー操作関数の呼び出しと、描画処理を ***application_run()*** に追加しました。なお、VBOやVAOといった用語が出てきていますが、詳細についてはBook4添付の付録を参照してください。

```c
application_result_t application_run(void) {
    application_result_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(%s) - Application is not initialized.", rslt_to_str(ret));
        goto cleanup;
    }

    if(!program_create()) {
        ret = APPLICATION_RUNTIME_ERROR;
        ERROR_MESSAGE("application_run(RUNTIME_ERROR) - Failed to create shader program.");
        goto cleanup;
    }

    GLuint vertex_array_id;

    glGenVertexArrays(1, &vertex_array_id);

    glBindVertexArray(vertex_array_id);

    static const GLfloat vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f,  1.0f, 0.0f,
    };

    GLuint vertexbuffer;

    glGenBuffers(1, &vertexbuffer);

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);

    glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    sizeof(GLfloat) * 3,
    (void*)0
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    GLFWwindow* window = (GLFWwindow*)platform_window_surface_get(s_app_state->platform_context);

    struct timespec  req = {0, 1000000};
    while(!s_app_state->window_should_close) {
        platform_result_t ret_event = platform_pump_messages(s_app_state->platform_context, on_window, on_key, on_mouse);
        if(PLATFORM_WINDOW_CLOSE == ret_event) {
            s_app_state->window_should_close = true;
            continue;
        } else if(PLATFORM_SUCCESS != ret_event) {
            ret = rslt_convert_platform(ret_event);
            WARN_MESSAGE("application_run(%s) - Failed to pump events.", rslt_to_str(ret));
            continue;
        }
        app_state_update();
        app_state_dispatch();
        app_state_clean();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(s_app_state->program_id);

        glViewport(0, 0, s_app_state->framebuffer_width, s_app_state->framebuffer_height);

        glBindVertexArray(vertex_array_id);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);

        platform_swap_buffers(s_app_state->platform_context);

        nanosleep(&req, NULL);
    }
cleanup:
    return ret;
}
```
