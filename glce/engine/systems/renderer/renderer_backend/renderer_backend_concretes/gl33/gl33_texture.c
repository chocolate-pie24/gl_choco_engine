/** @ingroup renderer
 *
 * @file gl33_texture.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のテクスチャ操作関数の実装
 *
 * @version 0.1
 * @date 2026-05-15
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_texture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_texture_vtable.h"

/**
 * @brief テクスチャGPU側リソース内部状態管理構造体
 *
 */
struct renderer_backend_texture {
    GLuint handle;          /**< テクスチャGPU側リソースハンドル */
    int32_t unit_number;    /**< シェーダーが参照するテクスチャ用スロット番号(GL_TEXTURE0などのenum値ではなく、GL_TEXTURE0 + unit_num_として使用される0始まりのtexture unit index) */

    texture_min_filter_config_t min_filter_config;  /**< テクスチャを縮小表示する際のピクセルの補間設定 */
    texture_mag_filter_config_t mag_filter_config;  /**< テクスチャを拡大表示する際のピクセルの補間設定 */
    texture_wrap_config_t wrap_config_s_axis;       /**< テクスチャがラップする箇所のピクセル設定(s軸) */
    texture_wrap_config_t wrap_config_t_axis;       /**< テクスチャがラップする箇所のピクセル設定(t軸) */
};

static renderer_backend_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_);
static renderer_backend_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_);
static renderer_backend_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_);
static renderer_backend_result_t gl33_texture_pixel_upload(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

static bool resolve_min_filter_config(texture_min_filter_config_t src_, GLint* dst_);
static bool resolve_mag_filter_config(texture_mag_filter_config_t src_, GLint* dst_);
static bool resolve_wrap_config(texture_wrap_config_t src_, GLint* dst_);

// OpenGLモック関数プロトタイプ宣言
static void mock_glGetIntegerv(GLenum pname_, GLint* data_);
static void mock_glGenTextures(GLsizei n_, GLuint* textures_);
static void mock_glActiveTexture(GLenum texture_);
static void mock_glBindTexture(GLenum target_, GLuint texture_);
static void mock_glTexParameteri(GLenum target_, GLenum pname_, GLint param_);
static void mock_glDeleteTextures(GLsizei n_, const GLuint* textures_);
static void mock_glPixelStorei(GLenum pname_, GLint param_);
static void mock_glTexImage2D(GLenum target_, GLint level_, GLint internalformat_, GLsizei width_, GLsizei height_, GLint border_, GLenum format_, GLenum type_, const void* data_);

static const renderer_texture_vtable_t s_gl33_texture_vtable = {
    .renderer_texture_create = gl33_texture_create,
    .renderer_texture_destroy = gl33_texture_destroy,
    .renderer_texture_bind = gl33_texture_bind,
    .renderer_texture_unbind = gl33_texture_unbind,
    .renderer_texture_pixel_upload = gl33_texture_pixel_upload,
};  /**< OpenGL3.3用テクスチャ操作仮想関数テーブル */

const renderer_texture_vtable_t* gl33_texture_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_texture_vtable;
}

/**
 * @brief テクスチャGPU側リソース構造体インスタンスのメモリを確保し、OpenGLテクスチャ設定を行い初期化する
 *
 * @param[in] unit_num_ シェーダーが参照するテクスチャ用スロット番号
 * @param[in] min_filter_config_ テクスチャ縮小表示の際の設定値
 * @param[in] mag_filter_config_ テクスチャ拡大表示の際の設定値
 * @param[in] wrap_config_s_axis_ テクスチャがラップする部分の表示設定値(s軸)
 * @param[in] wrap_config_t_axis_ テクスチャがラップする部分の表示設定値(t軸)
 * @param[out] texture_handle_ リソース確保、初期化対象テクスチャGPUリソース構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - texture_handle_ == NULL
 * - *texture_handle_ != NULL
 * - min_filter_config_が規定値外
 * - mag_filter_config_が規定値外
 * - wrap_config_s_axis_が規定値外
 * - wrap_config_t_axis_が規定値外
 * - unit_num_ < 0
 * @retval RENDERER_BACKEND_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_BACKEND_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    renderer_backend_texture_t* tmp = NULL;
    GLint min_filter = GL_NEAREST;
    GLint mag_filter = GL_NEAREST;
    GLint wrap_config_s_axis = GL_REPEAT;
    GLint wrap_config_t_axis = GL_REPEAT;
    GLint current_unit = GL_TEXTURE0;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "*texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_min_filter_config(min_filter_config_, &min_filter), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "min_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_mag_filter_config(mag_filter_config_, &mag_filter), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "mag_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_s_axis_, &wrap_config_s_axis), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_s_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_t_axis_, &wrap_config_t_axis), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_t_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(unit_num_ >= 0, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "unit_num_")

    ret_memory_system = memory_system_allocate(sizeof(renderer_backend_texture_t), MEMORY_TAG_RENDERER, (void**)&tmp);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = renderer_backend_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("gl33_texture_create(%s) - Failed to allocate memory for texture handle.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->handle = 0;
    tmp->unit_number = unit_num_;
    tmp->min_filter_config = min_filter_config_;
    tmp->mag_filter_config = mag_filter_config_;
    tmp->wrap_config_s_axis = wrap_config_s_axis_;
    tmp->wrap_config_t_axis = wrap_config_t_axis_;

    mock_glGetIntegerv(GL_ACTIVE_TEXTURE, &current_unit);

    mock_glGenTextures(1, &tmp->handle);
    mock_glActiveTexture(GL_TEXTURE0 + (GLenum)unit_num_);

    mock_glBindTexture(GL_TEXTURE_2D, tmp->handle);

    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_config_s_axis);   // s軸
    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_config_t_axis);   // t軸

    mock_glBindTexture(GL_TEXTURE_2D, 0);

    mock_glActiveTexture((GLenum)current_unit);

    *texture_handle_ = tmp;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief テクスチャGPUリソース構造体が保持するリソースを解放し、自身のメモリも解放する
 *
 * @note 本関数実行後、texture_handle_はNULLに初期化される
 * @note 2重destroy許可
 *
 * @param[in,out] texture_handle_ リソース解放対象構造体インスタンスへのダブルポインタ
 */
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_) {
    if(NULL == texture_handle_) {
        return;
    }
    if(NULL == *texture_handle_) {
        return;
    }
    mock_glDeleteTextures(1, &(*texture_handle_)->handle);

    memory_system_free((void*)*texture_handle_, sizeof(renderer_backend_texture_t), MEMORY_TAG_RENDERER);

    *texture_handle_ = NULL;
}

/**
 * @brief テクスチャをactiveにし、bindする
 *
 * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
 * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
 * - texture_handle_->handle == 0
 * - texture_handle_->unit_number < 0
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_bind", "texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_rslt_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_rslt_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->unit_number")

    mock_glActiveTexture(GL_TEXTURE0 + texture_handle_->unit_number);
    mock_glBindTexture(GL_TEXTURE_2D, texture_handle_->handle);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief テクスチャをactiveにし、unbindする
 *
 * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT texture_handle_ == NULL
 * @retval RENDERER_BACKEND_DATA_CORRUPTED 以下のいずれか
 * - texture_handle_->handle == 0
 * - texture_handle_->unit_number < 0
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_unbind", "texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_rslt_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_rslt_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->unit_number")

    mock_glActiveTexture(GL_TEXTURE0 + texture_handle_->unit_number);
    mock_glBindTexture(GL_TEXTURE_2D, 0);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 現在active / bindされているGL_TEXTURE_2Dに対してピクセルデータをGPUへ転送する
 *
 * @param width_ 転送ピクセルデータの幅
 * @param height_ 転送ピクセルデータの高さ
 * @param channel_count_ 転送ピクセルデータのチャンネルカウント(RGB or RGBAのみ許可)
 * @param pixels_ 転送ピクセルデータ
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - pixels_ == NULL
 * - width_ == 0
 * - height_ == 0
 * - channel_count_が3, 4以外
 * @retval RENDERER_BACKEND_SUCCESS 処理に成功し、正常終了
 */
static renderer_backend_result_t gl33_texture_pixel_upload(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != width_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != height_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "height_")

    if(3 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width_, (GLsizei)height_, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels_);
    } else if(4 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width_, (GLsizei)height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
    } else {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_texture_pixel_upload(%s) - Provided channel count is not valid.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    if(RENDERER_BACKEND_SUCCESS == ret) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
    return ret;
}

/**
 * @brief テクスチャ縮小表示時の表示設定値をGLCE設定値からOpenGL設定値に変換する
 *
 * @param src_ 変換元GLCE設定値
 * @param dst_ 変換先OpenGL設定値
 *
 * @retval true 変換成功
 * @retval false 変換失敗
 */
static bool resolve_min_filter_config(texture_min_filter_config_t src_, GLint* dst_) {
    bool ret = false;

    if(NULL == dst_) {
        return false;
    }

    switch(src_) {
    case TEXTURE_MIN_FILTER_CONFIG_NEAREST:
        *dst_ = GL_NEAREST;
        ret = true;
        break;
    case TEXTURE_MIN_FILTER_CONFIG_LINEAR:
        *dst_ = GL_LINEAR;
        ret = true;
        break;
    // case TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_NEAREST:
    //     *dst_ = GL_NEAREST_MIPMAP_NEAREST;
    //     ret = true;
    //     break;
    // case TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_NEAREST:
    //     *dst_ = GL_LINEAR_MIPMAP_NEAREST;
    //     ret = true;
    //     break;
    // case TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_LINEAR:
    //     *dst_ = GL_NEAREST_MIPMAP_LINEAR;
    //     ret = true;
    //     break;
    // case TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_LINEAR:
    //     *dst_ = GL_LINEAR_MIPMAP_LINEAR;
    //     ret = true;
    //     break;
    default:
        ret = false;
        break;
    }
    return ret;
}

/**
 * @brief テクスチャ拡大表示時の表示設定値をGLCE設定値からOpenGL設定値に変換する
 *
 * @param src_ 変換元GLCE設定値
 * @param dst_ 変換先OpenGL設定値
 *
 * @retval true 変換成功
 * @retval false 変換失敗
 */
static bool resolve_mag_filter_config(texture_mag_filter_config_t src_, GLint* dst_) {
    bool ret = false;

    if(NULL == dst_) {
        return false;
    }

    switch(src_) {
    case TEXTURE_MAG_FILTER_CONFIG_NEAREST:
        *dst_ = GL_NEAREST;
        ret = true;
        break;
    case TEXTURE_MAG_FILTER_CONFIG_LINEAR:
        *dst_ = GL_LINEAR;
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

/**
 * @brief テクスチャがラップする部分の表示設定値をGLCE設定値からOpenGL設定値に変換する
 *
 * @param src_ 変換元GLCE設定値
 * @param dst_ 変換先OpenGL設定値
 *
 * @retval true 変換成功
 * @retval false 変換失敗
 */
static bool resolve_wrap_config(texture_wrap_config_t src_, GLint* dst_) {
    bool ret = false;

    if(NULL == dst_) {
        return false;
    }

    switch(src_) {
    case TEXTURE_WRAP_CONFIG_REPEAT:
        *dst_ = GL_REPEAT;
        ret = true;
        break;
    case TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT:
        *dst_ = GL_MIRRORED_REPEAT;
        ret = true;
        break;
    case TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE:
        *dst_ = GL_CLAMP_TO_EDGE;
        ret = true;
        break;
    case TEXTURE_WRAP_CONFIG_CLAMP_TO_BORDER:
        *dst_ = GL_CLAMP_TO_BORDER;
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

static void NO_COVERAGE mock_glGetIntegerv(GLenum pname_, GLint* data_) {
    glGetIntegerv(pname_, data_);
}

static void NO_COVERAGE mock_glGenTextures(GLsizei n_, GLuint* textures_) {
    glGenTextures(n_, textures_);
}

static void NO_COVERAGE mock_glActiveTexture(GLenum texture_) {
    glActiveTexture(texture_);
}

static void NO_COVERAGE mock_glBindTexture(GLenum target_, GLuint texture_) {
    glBindTexture(target_, texture_);
}

static void NO_COVERAGE mock_glTexParameteri(GLenum target_, GLenum pname_, GLint param_) {
    glTexParameteri(target_, pname_, param_);
}

static void NO_COVERAGE mock_glDeleteTextures(GLsizei n_, const GLuint* textures_) {
    glDeleteTextures(n_, textures_);
}

static void NO_COVERAGE mock_glPixelStorei(GLenum pname_, GLint param_) {
    glPixelStorei(pname_, param_);
}

static void NO_COVERAGE mock_glTexImage2D(GLenum target_, GLint level_, GLint internalformat_, GLsizei width_, GLsizei height_, GLint border_, GLenum format_, GLenum type_, const void* data_) {
    glTexImage2D(target_, level_, internalformat_, width_, height_, border_, format_, type_, data_);
}
