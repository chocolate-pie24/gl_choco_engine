/** @ingroup renderer
 *
 * @file gl33_vao.c
 * @author chocolate-pie24
 * @brief OpenGL固有の型やAPIを使用せず、VAOを使用するためのラッパーAPIの実装
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
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vao.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vao_vtable.h"

/**
 * @brief VAOモジュール内部状態管理構造体
 *
 */
struct renderer_backend_vao {
    GLuint vao_handle;  /**< VAO */
};

static renderer_backend_result_t gl33_vao_create(renderer_backend_vao_t** vao_);
static void gl33_vao_destroy(renderer_backend_vao_t** vao_);
static renderer_backend_result_t gl33_vao_bind(const renderer_backend_vao_t* vao_);
static renderer_backend_result_t gl33_vao_unbind(void);
static renderer_backend_result_t gl33_vao_attribute_set(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

static void mock_glGenVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glBindVertexArray(GLuint array_);
static void mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_);
static void mock_glEnableVertexAttribArray(GLuint index_);

static const renderer_vao_vtable_t s_gl33_vao_vtable = {
    .vao_create = gl33_vao_create,
    .vao_destroy = gl33_vao_destroy,
    .vao_bind = gl33_vao_bind,
    .vao_unbind = gl33_vao_unbind,
    .vao_attribute_set = gl33_vao_attribute_set,
};  /**< OpenGL3.3用VAO操作仮想関数テーブル */

const renderer_vao_vtable_t* gl33_vao_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_vao_vtable;
}

/**
 * @brief VAO構造体インスタンスのメモリを確保し、VAOハンドルを生成する
 *
 * @param[out] vao_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - vao_がNULL
 * - *vao_が非NULL
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR メモリ確保時に不明なエラーが発生
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限を超過
 * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vao_create(renderer_backend_vao_t** vao_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    renderer_backend_vao_t* tmp = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_create", "vao_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_create", "vao_")

    ret_memory_system = memory_system_allocate(sizeof(renderer_backend_vao_t), MEMORY_TAG_RENDERER, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("gl33_vao_create(%s) - Failed to allocate memory for 'tmp'.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    mock_glGenVertexArrays(1, &tmp->vao_handle);
    *vao_ = tmp;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief renderer_backend_vao_t構造体インスタンスのメモリを解放し、OpenGLContext内のVAOも削除する
 *
 * @param[in,out] vao_ renderer_backend_vao_t構造体インスタンスへのダブルポインタ
 */
static void gl33_vao_destroy(renderer_backend_vao_t** vao_) {
    if(NULL == vao_) {
        goto cleanup;
    }
    if(NULL == *vao_) {
        goto cleanup;
    }

    if(RENDERER_BACKEND_SUCCESS != gl33_vao_unbind()) {
        WARN_MESSAGE("gl33_vao_destroy(RUNTIME_ERROR) - Failed to unbind vertex array.");
    }
    mock_glDeleteVertexArrays(1, &(*vao_)->vao_handle);
    memory_system_free(*vao_, sizeof(renderer_backend_vao_t), MEMORY_TAG_RENDERER);

    *vao_ = NULL;

cleanup:
    return;
}

/**
 * @brief glBindVertexArray APIのラッパーAPI
 * @note 当面はglGetErrorをAPI個別に実行するつもりはないので成功するが、将来的に個別にエラー処理を行う可能性を考慮し、返り値をエラーコードにする
 *
 * @param[in] vao_ bind対象vao
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - vao_ == NULL
 * @retval RENDERER_BACKEND_BAD_OPERATION 未初期化のvao_が渡された
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vao_bind(const renderer_backend_vao_t* vao_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_bind", "vao_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vao_->vao_handle, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_vao_bind", "vao_->vao_handle")

    mock_glBindVertexArray(vao_->vao_handle);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief VAOアンバインド処理
 *
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vao_unbind(void) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    mock_glBindVertexArray(0);

    ret = RENDERER_BACKEND_SUCCESS;

    return ret;
}

/**
 * @brief OpenGL3.3用VAOアトリビュート設定
 *
 * @param[in] layout_ 設定対象変数のlayoutロケーション番号
 * @param[in] size_ 頂点属性のコンポーネントの数
 * @param[in] type_ 頂点属性のデータ型
 * @param[in] normalized_ true: アクセス時に固定小数点データ値を正規化する / false: 正規化しない
 * @param[in] stride_ 連続する頂点属性間のバイトオフセット
 * @param[in] offset_ 設定対象頂点属性が格納されているバイトオフセット
 *
 * @retval RENDERER_BACKEND_RUNTIME_ERROR type_が規定値外
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_vao_attribute_set(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    switch(type_) {
    case RENDERER_TYPE_FLOAT:
        mock_glVertexAttribPointer(layout_, size_, GL_FLOAT, normalized_ ? GL_TRUE : GL_FALSE, (GLsizei)stride_, (void*)offset_);
        break;
    case RENDERER_TYPE_UNSIGNED_BYTE:
        mock_glVertexAttribPointer(layout_, size_, GL_UNSIGNED_BYTE, normalized_ ? GL_TRUE : GL_FALSE, (GLsizei)stride_, (void*)offset_);
        break;
    case RENDERER_TYPE_BYTE:
        mock_glVertexAttribPointer(layout_, size_, GL_BYTE, normalized_ ? GL_TRUE : GL_FALSE, (GLsizei)stride_, (void*)offset_);
        break;
    default:
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        goto cleanup;
    }
    mock_glEnableVertexAttribArray(layout_);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

static void NO_COVERAGE mock_glGenVertexArrays(GLsizei n_, GLuint* array_) {
    glGenVertexArrays(n_, array_);
}

static void NO_COVERAGE mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_) {
    glDeleteVertexArrays(n_, array_);
}

static void NO_COVERAGE mock_glBindVertexArray(GLuint array_) {
    glBindVertexArray(array_);
}

static void NO_COVERAGE mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_) {
    glVertexAttribPointer(index_, size_, type_, normalized_, stride_, pointer_);
}

static void NO_COVERAGE mock_glEnableVertexAttribArray(GLuint index_) {
    glEnableVertexAttribArray(index_);
}
