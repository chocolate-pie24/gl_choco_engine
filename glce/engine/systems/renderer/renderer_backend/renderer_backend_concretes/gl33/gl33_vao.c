// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup renderer
 *
 * @file gl33_vao.c
 * @author chocolate-pie24
 * @brief OpenGL 3.3によるRenderer Backend VAOのConcrete実装
 *
 * @details
 * Renderer Backend VAOのvtableに対応する
 * OpenGL 3.3固有の処理を提供する。
 *
 * VAO Handleの生成、削除、bind、unbind、および
 * Vertex Attributeの設定を担当する。
 *
 * VAOの内部表現にはOpenGLのGLuintを使用する。
 *
 * 現在の実装では、個々のOpenGL API呼び出しに対して
 * glGetError()による実行結果の検証を行わない。
 *
 * @date 2025-12-19
 *
 */
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vao.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

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

static renderer_backend_result_t gl33_vao_create(renderer_backend_vao_t** out_vao_);
static void gl33_vao_destroy(renderer_backend_vao_t** vao_);
static renderer_backend_result_t gl33_vao_bind(const renderer_backend_vao_t* vao_);
static renderer_backend_result_t gl33_vao_unbind(void);
static renderer_backend_result_t gl33_vao_attribute_set(uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_);

static void mock_glGenVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glDeleteVertexArrays(GLsizei n_, GLuint* array_);
static void mock_glBindVertexArray(GLuint array_);
static void mock_glVertexAttribPointer(GLuint index_, GLint size_, GLenum type_, GLboolean normalized_, GLsizei stride_, const void * pointer_);
static void mock_glEnableVertexAttribArray(GLuint index_);

static bool gl33_vao_is_valid(const renderer_backend_vao_t* vao_);

static const renderer_vao_vtable_t s_gl33_vao_vtable = {
    .vao_create = gl33_vao_create,
    .vao_destroy = gl33_vao_destroy,
    .vao_bind = gl33_vao_bind,
    .vao_unbind = gl33_vao_unbind,
    .vao_attribute_set = gl33_vao_attribute_set,
    .vao_is_valid = gl33_vao_is_valid,
};  /**< OpenGL3.3用VAO操作仮想関数テーブル */

const renderer_vao_vtable_t* gl33_vao_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_vao_vtable;
}

/**
 * @brief VAO管理Objectを確保し、OpenGL VAO Handleの生成を要求する
 *
 * @details
 * General Allocatorからrenderer_backend_vao_tのStorageを取得し、
 * glGenVertexArrays()を呼び出してVAO Handleの生成を要求する。
 *
 * 現在の実装では、glGenVertexArrays()の実行結果および
 * 生成されたVAO Handleの妥当性を検証しない。
 *
 * したがって、SUCCESSはGeneral Allocatorによる
 * 管理Objectの確保とOpenGL APIの呼び出しが完了したことを表し、
 * OpenGL側のResource Stateの正常性までは保証しない。
 *
 * @param[out] out_vao_
 * 生成されたVAO管理Objectの格納先。
 * 呼び出し前に*out_vao_はNULLでなければならない。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * VAO管理Objectの生成処理が成功した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * out_vao_がNULL、または*out_vao_がNULLではない。
 * 下位Allocator由来の引数不正も含む。
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * 管理ObjectのStorageを確保できなかった
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * 下位Allocatorの操作条件を満たしていない
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * 下位Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * 下位Allocatorで算術overflowが検出された
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * 下位Allocatorで内部データ破損が検出された
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * 下位Allocatorから未定義または変換対象外の結果を受け取った
 */
static renderer_backend_result_t gl33_vao_create(renderer_backend_vao_t** out_vao_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    renderer_backend_vao_t* tmp_vao = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_create", "out_vao_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_create", "*out_vao_")

    ret_general_allocator = general_allocator_allocate(sizeof(renderer_backend_vao_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_vao);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = renderer_backend_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("gl33_vao_create(%s) - general_allocator_allocate failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    mock_glGenVertexArrays(1, &tmp_vao->vao_handle);
    *out_vao_ = tmp_vao;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief VAO管理Objectを破棄し、OpenGL VAO Handleの削除を要求する
 *
 * @details
 * 現在BindされているVAOのunbindを試みた後、
 * glDeleteVertexArrays()によって対象VAO Handleの削除を要求する。
 *
 * unbindが失敗した場合はWarningを出力するが、
 * 後続のVAO削除処理と管理Objectのメモリ解放を継続する。
 *
 * 管理Objectの解放にはGeneral Allocatorを使用し、
 * 最後に*vao_をNULLへ設定する。
 *
 * vao_または*vao_がNULLの場合は、何も行わない。
 *
 * 現在の実装では、OpenGL側の削除結果および
 * General Allocatorによる解放結果をCallerへ通知しない。
 *
 * @param[in,out] vao_
 * 破棄対象のVAO管理Objectを保持するPointerの格納先
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
    general_allocator_free((void**)vao_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

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

    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_vao_bind", "vao_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vao_->vao_handle, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_result_to_str(RENDERER_BACKEND_BAD_OPERATION), "gl33_vao_bind", "vao_->vao_handle")

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

/**
 * @brief OpenGL3.3用VAO GPUリソース内部状態管理構造体の妥当性を検証する
 *
 * @details
 * VAO管理構造体がNULLではないことと、
 * 保持するOpenGL VAOハンドルが0ではないことを確認する。
 *
 * @note OpenGL APIを使用したGPUリソースの存在確認や、
 * VAOのBind状態の確認は行わない。
 *
 * @param[in] vao_ 検証対象のVAO GPUリソース内部状態管理構造体インスタンス
 *
 * @retval true 内部状態が妥当
 * @retval false 以下のいずれか
 * - vao_がNULL
 * - vao_handleが0
 */
static bool gl33_vao_is_valid(const renderer_backend_vao_t* vao_) {
    if(NULL == vao_) {
        return false;
    }
    if(0 == vao_->vao_handle) {
        return false;
    }

    return true;
}
