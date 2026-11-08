// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file gl33_texture.c
 * @author chocolate-pie24
 * @brief OpenGL3.3用のテクスチャ操作関数の実装
 *
 * @date 2026-05-15
 *
 */
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_texture.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

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

static renderer_backend_result_t gl33_texture_create(int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** out_texture_handle_);
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_);
static renderer_backend_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_);
static renderer_backend_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_);
static renderer_backend_result_t gl33_texture_pixel_upload(int32_t width_, int32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

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

static bool gl33_texture_is_valid(const renderer_backend_texture_t* texture_handle_);

static const renderer_texture_vtable_t s_gl33_texture_vtable = {
    .renderer_texture_create = gl33_texture_create,
    .renderer_texture_destroy = gl33_texture_destroy,
    .renderer_texture_bind = gl33_texture_bind,
    .renderer_texture_unbind = gl33_texture_unbind,
    .renderer_texture_pixel_upload = gl33_texture_pixel_upload,
    .renderer_texture_is_valid = gl33_texture_is_valid,
};  /**< OpenGL3.3用テクスチャ操作仮想関数テーブル */

const renderer_texture_vtable_t* gl33_texture_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_texture_vtable;
}

/**
 * @brief OpenGL 3.3 Textureの管理Objectを生成する
 *
 * @details
 * General Allocatorから管理ObjectのStorageを確保し、
 * glGenTextures()によってTexture Handleの生成を要求する。
 *
 * 指定されたTexture UnitをactiveにしてTextureをbindし、
 * Min / Mag FilterおよびS / T軸のWrap設定を適用する。
 *
 * 処理開始時のActive Texture Unitは保存・復元するが、
 * 設定対象UnitのGL_TEXTURE_2D Bindingは0に変更する。
 * 処理前のBindingは復元しない。
 *
 * 現在の実装ではOpenGL APIの実行結果や、
 * 生成されたTexture Handleが0ではないことを検証しない。
 * SUCCESSはOpenGL Resourceの正常性まで保証しない。
 *
 * @param[in] texture_unit_index_
 * Texture Unitの0始まりのIndex。
 * 0以上であることを検査するが、
 * OpenGL実装のTexture Unit上限は確認しない。
 *
 * @param[in] min_filter_config_
 * 縮小Filter設定。NEARESTまたはLINEARを許可する。
 *
 * @param[in] mag_filter_config_
 * 拡大Filter設定。NEARESTまたはLINEARを許可する。
 *
 * @param[in] wrap_config_s_axis_
 * S軸のWrap設定。
 *
 * @param[in] wrap_config_t_axis_
 * T軸のWrap設定。
 *
 * @param[out] out_texture_handle_
 * 生成したTexture管理Objectの格納先。
 * 呼び出し前に*out_texture_handle_はNULLであること。
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * 管理Object生成とOpenGL設定APIの呼び出しが完了した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * Output Pointer、Texture Unit Index、または
 * Filter / Wrap設定が入力条件を満たさない。
 * General Allocator由来の引数不正も含む。
 *
 * @retval RENDERER_BACKEND_BAD_OPERATION
 * General Allocatorの操作条件を満たしていない
 *
 * @retval RENDERER_BACKEND_NO_MEMORY
 * 管理ObjectのStorageを確保できなかった
 *
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED
 * General Allocatorの管理上限を超えた
 *
 * @retval RENDERER_BACKEND_OVERFLOW
 * General Allocatorで算術Overflowを検出した
 *
 * @retval RENDERER_BACKEND_DATA_CORRUPTED
 * General Allocatorで内部データ破損を検出した
 *
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR
 * General Allocatorから未定義の結果を受け取った
 */
static renderer_backend_result_t gl33_texture_create(int32_t texture_unit_index_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** out_texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    renderer_backend_texture_t* tmp_texture = NULL;
    GLint min_filter = GL_NEAREST;
    GLint mag_filter = GL_NEAREST;
    GLint wrap_config_s_axis = GL_REPEAT;
    GLint wrap_config_t_axis = GL_REPEAT;
    GLint current_unit = GL_TEXTURE0;

    IF_ARG_NULL_GOTO_CLEANUP(out_texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "out_texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "*out_texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_min_filter_config(min_filter_config_, &min_filter), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "min_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_mag_filter_config(mag_filter_config_, &mag_filter), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "mag_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_s_axis_, &wrap_config_s_axis), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_s_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_t_axis_, &wrap_config_t_axis), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_t_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_unit_index_ >= 0, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_create", "texture_unit_index_")

    ret_general_allocator = general_allocator_allocate(sizeof(renderer_backend_texture_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_texture);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = renderer_backend_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("gl33_texture_create(%s) - general_allocator_allocate failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
    tmp_texture->handle = 0;
    tmp_texture->unit_number = texture_unit_index_;
    tmp_texture->min_filter_config = min_filter_config_;
    tmp_texture->mag_filter_config = mag_filter_config_;
    tmp_texture->wrap_config_s_axis = wrap_config_s_axis_;
    tmp_texture->wrap_config_t_axis = wrap_config_t_axis_;

    mock_glGetIntegerv(GL_ACTIVE_TEXTURE, &current_unit);

    mock_glGenTextures(1, &tmp_texture->handle);
    mock_glActiveTexture(GL_TEXTURE0 + (GLenum)texture_unit_index_);

    mock_glBindTexture(GL_TEXTURE_2D, tmp_texture->handle);

    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_config_s_axis);   // s軸
    mock_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_config_t_axis);   // t軸

    mock_glBindTexture(GL_TEXTURE_2D, 0);

    mock_glActiveTexture((GLenum)current_unit);

    *out_texture_handle_ = tmp_texture;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief OpenGL Texture Handleと管理Objectを破棄する
 *
 * @details
 * glDeleteTextures()によってTexture Handleの削除を要求し、
 * General Allocatorによって管理ObjectのStorageを解放する。
 *
 * texture_handle_または*texture_handle_がNULLの場合は
 * 何も行わない。反復呼び出しを許容する。
 *
 * General Allocatorのfree結果およびOpenGL APIの実行結果は
 * 検証・通知しない。
 *
 * 現在の実装では、これらの結果に関係なく、
 * 解放処理後に*texture_handle_をNULLへ設定する。
 *
 * @param[in,out] texture_handle_
 * 破棄対象のTexture管理Objectを保持するPointerの格納先
 */
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_) {
    if(NULL == texture_handle_) {
        return;
    }
    if(NULL == *texture_handle_) {
        return;
    }
    mock_glDeleteTextures(1, &(*texture_handle_)->handle);

    general_allocator_free((void**)texture_handle_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

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

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_bind", "texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_result_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_result_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->unit_number")

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

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_unbind", "texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_result_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_BACKEND_DATA_CORRUPTED, renderer_backend_result_to_str(RENDERER_BACKEND_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->unit_number")

    mock_glActiveTexture(GL_TEXTURE0 + texture_handle_->unit_number);
    mock_glBindTexture(GL_TEXTURE_2D, 0);

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief 現在bindされている2D TextureへPixel Dataを転送する
 *
 * @details
 * 現在のActive Texture UnitでGL_TEXTURE_2Dにbindされている
 * Textureを対象として、glTexImage2D()を呼び出す。
 *
 * channel_count_が3の場合はGL_RGB、
 * 4の場合はGL_RGBAを使用する。
 * Pixel Componentの型はGL_UNSIGNED_BYTEとする。
 *
 * Pixel DataのRow Alignmentに対応するため、
 * 転送前にGL_UNPACK_ALIGNMENTを1に設定する。
 *
 * 成功経路ではGL_UNPACK_ALIGNMENTを4に設定する。
 * 処理前の値を保存・復元するわけではない。
 *
 * 現在の実装ではOpenGL APIの実行結果を検証しない。
 * また、対象Textureのbind状態も検証しない。
 *
 * @warning
 * 呼び出し前に対象Texture Unitをactiveにし、
 * 対象の2D Textureをbindしておくこと。
 *
 * @pre
 * pixels_は、width_ * height_ * channel_count_ byteの
 * Pixel Dataを読み取り可能なStorageを指すこと。
 *
 * @param[in] width_ Pixel Dataの幅。0より大きいこと。
 * @param[in] height_ Pixel Dataの高さ。0より大きいこと。
 * @param[in] channel_count_ Channel数。3または4。
 * @param[in] pixels_ 転送元Pixel Data
 *
 * @retval RENDERER_BACKEND_SUCCESS
 * OpenGLへの転送要求を実行した
 *
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT
 * pixels_がNULL、width_またはheight_が0以下、
 * あるいはchannel_count_が3と4以外の場合
 */
static renderer_backend_result_t gl33_texture_pixel_upload(int32_t width_, int32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < width_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < height_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "height_")

    if(3 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width_, (GLsizei)height_, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels_);
    } else if(4 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width_, (GLsizei)height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
    } else {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_texture_pixel_upload(%s) - Provided channel count is not valid.", renderer_backend_result_to_str(ret));
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

/**
 * @brief OpenGL3.3用テクスチャGPUリソース内部状態管理構造体の妥当性を検証する
 *
 * @details
 * テクスチャ管理構造体が保持する以下の値を検証する。
 *
 * - テクスチャユニット番号
 * - OpenGLテクスチャハンドル
 * - Min Filter設定
 * - Mag Filter設定
 * - S軸のWrap設定
 * - T軸のWrap設定
 *
 * @note テクスチャユニット番号については負数でないことを確認する。
 * OpenGL実装がサポートするテクスチャユニット数の上限は検証しない。
 *
 * @note OpenGL APIを使用したGPUリソースの存在確認や、
 * テクスチャのBind状態の確認は行わない。
 *
 * @param[in] texture_handle_ 検証対象のテクスチャGPUリソース内部状態管理構造体インスタンス
 *
 * @retval true 内部状態が妥当
 * @retval false 以下のいずれか
 * - texture_handle_がNULL
 * - unit_numberが負数
 * - handleが0
 * - min_filter_configが不正
 * - mag_filter_configが不正
 * - wrap_config_s_axisが不正
 * - wrap_config_t_axisが不正
 */
static bool gl33_texture_is_valid(const renderer_backend_texture_t* texture_handle_) {
    if(NULL == texture_handle_) {
        return false;
    }
    if(0 > texture_handle_->unit_number) {
        return false;
    }
    if(0 == texture_handle_->handle) {
        return false;
    }
    if(!texture_min_filter_config_is_valid(texture_handle_->min_filter_config)) {
        return false;
    }
    if(!texture_mag_filter_config_is_valid(texture_handle_->mag_filter_config)) {
        return false;
    }
    if(!texture_wrap_config_is_valid(texture_handle_->wrap_config_s_axis)) {
        return false;
    }
    if(!texture_wrap_config_is_valid(texture_handle_->wrap_config_t_axis)) {
        return false;
    }
    return true;
}
