/** @ingroup renderer
 *
 * @file gl33_vbo.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VBOを使用するためのラッパーAPIの実装
 *
 * @version 0.1
 * @date 2025-12-19
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vbo.h"

#include <stddef.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vbo_vtable.h"

/**
 * @brief VBOモジュール内部状態管理構造体
 *
 */
struct renderer_backend_vbo {
    GLuint vbo_handle;  /**< VBO */
};

static renderer_backend_result_t gl33_vbo_create(renderer_backend_vbo_t** vbo_);
static void gl33_vbo_destroy(renderer_backend_vbo_t** vbo_);
static renderer_backend_result_t gl33_vbo_bind(const renderer_backend_vbo_t* vbo_);
static renderer_backend_result_t gl33_vbo_unbind(void);
static renderer_backend_result_t gl33_vbo_vertex_load(size_t load_size_, const void* load_data_, buffer_usage_t usage_);
static renderer_backend_result_t gl33_vbo_vertex_subload(size_t offset_, size_t size_, const void* load_data_);

static void mock_glGenBuffers(GLsizei n_, GLuint* buffer_);
static void mock_glBindBuffer(GLenum target_, GLuint buffer_);
static void mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_);
static void mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_);
static void mock_glBufferSubData(GLenum target, GLintptr offset_, GLsizeiptr size_, const void * data_);

static const renderer_vbo_vtable_t s_gl33_vbo_vtable = {
    .vbo_create = gl33_vbo_create,
    .vbo_destroy = gl33_vbo_destroy,
    .vbo_bind = gl33_vbo_bind,
    .vbo_unbind = gl33_vbo_unbind,
    .vbo_vertex_load = gl33_vbo_vertex_load,
    .vbo_vertex_subload = gl33_vbo_vertex_subload,
};  /**< OpenGL3.3用VBO操作仮想関数テーブル */

const renderer_vbo_vtable_t* gl33_vbo_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_vbo_vtable;
}

/**
 * @brief VBO構造体インスタンスのメモリを確保し、VBOハンドルを生成する
 *
 * @param[out] vbo_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - vbo_がNULL
 * - *vbo_が非NULL
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vbo_create(renderer_backend_vbo_t** vbo_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    renderer_backend_vbo_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_create", "vbo_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_create", "vbo_")

    ret_memory_system = memory_system_allocate(sizeof(renderer_backend_vbo_t), MEMORY_TAG_RENDERER, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("gl33_vbo_create(%s) - gl33_vbo_create failed.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glGenBuffers(1, &tmp->vbo_handle);

    *vbo_ = tmp;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief renderer_backend_vbo_t構造体インスタンスのメモリを解放し、OpenGLContext内のVBOも削除する
 *
 * @param[in,out] vbo_ renderer_backend_vbo_t構造体インスタンスへのダブルポインタ
 */
static void gl33_vbo_destroy(renderer_backend_vbo_t** vbo_) {
    if(NULL == vbo_) {
        goto cleanup;
    }
    if(NULL == *vbo_) {
        goto cleanup;
    }
    mock_glDeleteBuffers(1, &(*vbo_)->vbo_handle);

    memory_system_free(*vbo_, sizeof(renderer_backend_vbo_t), MEMORY_TAG_RENDERER);

    *vbo_ = NULL;

cleanup:
    return;
}

/**
 * @brief glBindBuffer APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param[in] vbo_ bind対象vbo
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT vbo_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 未初期化のvbo_が渡された
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vbo_bind(const renderer_backend_vbo_t* vbo_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_bind", "vbo_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vbo_->vbo_handle, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_vbo_bind", "vbo_->vbo_handle")

    mock_glBindBuffer(GL_ARRAY_BUFFER, vbo_->vbo_handle);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief VBO unbind処理
 *
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vbo_unbind(void) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    mock_glBindBuffer(GL_ARRAY_BUFFER, 0);

    ret = RENDERER_BACKEND_SUCCESS;

    return ret;
}

/**
 * @brief GPU側頂点情報格納領域を生成し、頂点情報を転送する
 *
 * @warning 本APIを呼び出す前に必ず対象のVBOをbindしておくこと
 * @note load_data_ == NULLの場合は頂点情報格納領域の生成のみを行い、頂点情報の転送は行わない
 *
 * @param[in] load_size_ 頂点情報格納領域サイズ(byte)
 * @param[in] load_data_ 転送頂点情報配列へのポインタ
 * @param[in] usage_ バッファ使用方法種別
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT load_size_ == 0
 * @retval RENDERER_BACKEND_RUNTIME_ERROR 規定値外のusage_
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vbo_vertex_load(size_t load_size_, const void* load_data_, buffer_usage_t usage_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_FALSE_GOTO_CLEANUP(0 != load_size_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_vertex_load", "load_size_")

    switch(usage_) {
    case BUFFER_USAGE_STATIC:
        mock_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)load_size_, load_data_, GL_STATIC_DRAW);
        break;
    case BUFFER_USAGE_DYNAMIC:
        mock_glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)load_size_, load_data_, GL_DYNAMIC_DRAW);
        break;
    default:
        ERROR_MESSAGE("gl33_vbo_vertex_load(%s) - Provided usage_ is not valid.", renderer_backend_rslt_to_str(RENDERER_BACKEND_RUNTIME_ERROR));
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        goto cleanup;
    }

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 生成済みのGPU側頂点情報格納領域に対し、転送位置を指定して頂点情報を転送する
 *
 * @warning 本APIを呼び出す前に必ず対象のVBOをbindしておくこと
 *
 * @param[in] offset_ 頂点情報格納領域の先頭から転送開始位置までのオフセット(byte)
 * @param[in] size_ 頂点情報転送サイズ(byte)
 * @param[in] load_data_ 転送する頂点情報配列へのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - load_data_ == NULL
 * - size_ == 0
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vbo_vertex_subload(size_t offset_, size_t size_, const void* load_data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(load_data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_vertex_subload", "load_data_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vbo_vertex_subload", "size_")

    mock_glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)offset_, (GLsizeiptr)size_, load_data_);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static void NO_COVERAGE mock_glGenBuffers(GLsizei n_, GLuint* buffer_) {
    glGenBuffers(n_, buffer_);
}

static void NO_COVERAGE mock_glBindBuffer(GLenum target_, GLuint buffer_) {
    glBindBuffer(target_, buffer_);
}

static void NO_COVERAGE mock_glDeleteBuffers(GLsizei n_, const GLuint* buffer_) {
    glDeleteBuffers(n_, buffer_);
}

static void NO_COVERAGE mock_glBufferData(GLenum target_, GLsizeiptr size_, const void* data_, GLenum usage_) {
    glBufferData(target_, size_, data_, usage_);
}

static void NO_COVERAGE mock_glBufferSubData(GLenum target, GLintptr offset_, GLsizeiptr size_, const void * data_) {
    glBufferSubData(target, offset_, size_, data_);
}
