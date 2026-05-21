/** @ingroup gl33
 *
 * @file concrete_texture.c
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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_texture.h"

#include "engine/systems/renderer/renderer_core/renderer_memory.h"
#include "engine/systems/renderer/renderer_core/renderer_err_utils.h"
#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

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

static renderer_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_);
static renderer_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_unit_, int32_t* out_texture_internal_handle_);
static renderer_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_);
static renderer_result_t gl33_texture_pixel_upload(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_);

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

// #define TEST_BUILD

#ifdef TEST_BUILD
#include <assert.h>

#include "test_controller.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/test_concrete_texture.h"

#include "engine/core/memory/test_choco_memory.h"

#include "engine/core/memory/choco_memory.h"

// concrete_texture用モジュール専用テスト制御構造体定義
/**
 * @brief void型の関数を強制的にNo-Opで終了させるための設定値構造体
 *
 */
typedef struct test_call_control_no_op {
    uint32_t call_count;    /**< 関数呼び出し回数 */
    uint32_t fail_on_call;  /**< 関数を何回目の呼び出しでエラーにさせるかの設定(0なら無効で通常処理、1以上の場合はNo-Opで関数を終了させる) */
} test_call_control_no_op_t;

// 外部公開APIテスト設定

// プライベート関数テスト設定
static test_call_control_t s_test_config_gl33_texture_create;               /**< gl33_texture_create()テスト設定 */
static test_call_control_no_op_t s_test_config_gl33_texture_destroy;        /**< gl33_texture_destroy()テスト設定 */
static test_call_control_t s_test_config_gl33_texture_bind;                 /**< gl33_texture_bind()テスト設定 */
static test_call_control_t s_test_config_gl33_texture_unbind;               /**< gl33_texture_unbind()テスト設定 */
static test_call_control_t s_test_config_gl33_texture_pixel_upload;         /**< gl33_texture_pixel_upload()テスト設定 */
static test_call_control_bool_t s_test_config_resolve_min_filter_config;     /**< resolve_min_filter_config()テスト設定 */
static test_call_control_bool_t s_test_config_resolve_mag_filter_config;     /**< resolve_mag_filter_config()テスト設定 */
static test_call_control_bool_t s_test_config_resolve_wrap_config;           /**< resolve_wrap_config()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glGetIntegerv;          /**< mock_glGetIntegerv()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glGenTextures;          /**< mock_glGenTextures()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glActiveTexture;        /**< mock_glActiveTexture()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glBindTexture;          /**< mock_glBindTexture()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glTexParameteri;        /**< mock_glTexParameteri()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glDeleteTextures;       /**< mock_glDeleteTextures()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glPixelStorei;          /**< mock_glPixelStorei()テスト設定 */
static test_call_control_no_op_t s_test_config_mock_glTexImage2D;           /**< mock_glTexImage2D()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_gl33_texture_create(void);
static void test_gl33_texture_destroy(void);
static void test_gl33_texture_bind(void);
static void test_gl33_texture_unbind(void);
static void test_gl33_texture_pixel_upload(void);
static void test_resolve_min_filter_config(void);
static void test_resolve_mag_filter_config(void);
static void test_resolve_wrap_config(void);

// テスト用ヘルパー関数
static void test_call_control_no_op_reset(test_call_control_no_op_t* config_);
#endif

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
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - texture_handle_ == NULL
 * - *texture_handle_ != NULL
 * - min_filter_config_が規定値外
 * - mag_filter_config_が規定値外
 * - wrap_config_s_axis_が規定値外
 * - wrap_config_t_axis_が規定値外
 * - unit_num_ < 0
 * @retval RENDERER_BAD_OPERATION メモリシステム未初期化
 * @retval RENDERER_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ確保失敗
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_texture_create.call_count++;
    if(s_test_config_gl33_texture_create.fail_on_call != 0) {
        if(s_test_config_gl33_texture_create.call_count == s_test_config_gl33_texture_create.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_texture_create.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_texture_t* tmp = NULL;
    GLint min_filter = GL_NEAREST;
    GLint mag_filter = GL_NEAREST;
    GLint wrap_config_s_axis = GL_REPEAT;
    GLint wrap_config_t_axis = GL_REPEAT;
    GLint current_unit = GL_TEXTURE0;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "*texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_min_filter_config(min_filter_config_, &min_filter), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "min_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_mag_filter_config(mag_filter_config_, &mag_filter), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "mag_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_s_axis_, &wrap_config_s_axis), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_s_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolve_wrap_config(wrap_config_t_axis_, &wrap_config_t_axis), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_t_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(unit_num_ >= 0, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "unit_num_")

    ret = renderer_mem_allocate(sizeof(renderer_backend_texture_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_texture_create(%s) - Failed to allocate memory for texture handle.", renderer_rslt_to_str(ret));
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

    ret = RENDERER_SUCCESS;

cleanup:
#ifdef TEST_BUILD
    // NOTE: 将来的に仕様変更でrenderer_mem_allocate成功した後で失敗することを想定し、cleanup漏れ検出を追加
    // ここはカバレッジ到達不可だけど許容する
    if(RENDERER_SUCCESS != ret && NULL != tmp) {
        assert(false);
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_gl33_texture_destroy.call_count++;
    if(s_test_config_gl33_texture_destroy.fail_on_call != 0) {
        if(s_test_config_gl33_texture_destroy.call_count == s_test_config_gl33_texture_destroy.fail_on_call) {
            return;
        }
    }
#endif
    if(NULL == texture_handle_) {
        return;
    }
    if(NULL == *texture_handle_) {
        return;
    }
    mock_glBindTexture(GL_TEXTURE_2D, 0);

    mock_glDeleteTextures(1, &(*texture_handle_)->handle);

    renderer_mem_free((void*)*texture_handle_, sizeof(renderer_backend_texture_t));

    *texture_handle_ = NULL;
}

/**
 * @brief テクスチャをactiveにし、bindする
 *
 * @note 呼び出し側が保持している現在のtexture unit / texture handleキャッシュとtexture_handle_が保持するunit / handleと等しい場合はactive化 / bindを行わない
 *
 * @param[in] texture_handle_ bind対象テクスチャハンドル保有構造体インスタンスへのポインタ
 * @param[in,out] out_texture_unit_ 現在activeになっているテクスチャユニット番号
 * @param[in,out] out_texture_internal_handle_ 現在Bindされているテクスチャハンドル
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - texture_handle_ == NULL
 * - out_texture_unit_ == NULL
 * - out_texture_internal_handle_ == NULL
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - texture_handle_->handle == 0
 * - texture_handle_->unit_number < 0
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_unit_, int32_t* out_texture_internal_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_texture_bind.call_count++;
    if(s_test_config_gl33_texture_bind.fail_on_call != 0) {
        if(s_test_config_gl33_texture_bind.call_count == s_test_config_gl33_texture_bind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_texture_bind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_bind", "texture_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_unit_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_bind", "out_texture_unit_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_internal_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_bind", "out_texture_internal_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_DATA_CORRUPTED, renderer_rslt_to_str(RENDERER_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_DATA_CORRUPTED, renderer_rslt_to_str(RENDERER_DATA_CORRUPTED), "gl33_texture_bind", "texture_handle_->unit_number")

    if(*out_texture_unit_ != texture_handle_->unit_number || *out_texture_internal_handle_ != texture_handle_->handle) {
        mock_glActiveTexture(GL_TEXTURE0 + texture_handle_->unit_number);
        mock_glBindTexture(GL_TEXTURE_2D, texture_handle_->handle);
        *out_texture_unit_ = texture_handle_->unit_number;
        *out_texture_internal_handle_ = (int32_t)texture_handle_->handle;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief テクスチャをactiveにし、unbindする
 *
 * @param[in] texture_handle_ unbind対象テクスチャGPUリソース構造体インスタンスへのポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT texture_handle_ == NULL
 * @retval RENDERER_DATA_CORRUPTED 以下のいずれか
 * - texture_handle_->handle == 0
 * - texture_handle_->unit_number < 0
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_) {
#ifdef TEST_BUILD
    s_test_config_gl33_texture_unbind.call_count++;
    if(s_test_config_gl33_texture_unbind.fail_on_call != 0) {
        if(s_test_config_gl33_texture_unbind.call_count == s_test_config_gl33_texture_unbind.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_texture_unbind.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_unbind", "texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != texture_handle_->handle, ret, RENDERER_DATA_CORRUPTED, renderer_rslt_to_str(RENDERER_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->handle")
    IF_ARG_FALSE_GOTO_CLEANUP(0 <= texture_handle_->unit_number, ret, RENDERER_DATA_CORRUPTED, renderer_rslt_to_str(RENDERER_DATA_CORRUPTED), "gl33_texture_unbind", "texture_handle_->unit_number")

    mock_glActiveTexture(GL_TEXTURE0 + texture_handle_->unit_number);
    mock_glBindTexture(GL_TEXTURE_2D, 0);

    ret = RENDERER_SUCCESS;

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
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - pixels_ == NULL
 * - width_ == 0
 * - height_ == 0
 * - channel_count_が3, 4以外
 * @retval RENDERER_SUCCESS 処理に成功し、正常終了
 */
static renderer_result_t gl33_texture_pixel_upload(uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
#ifdef TEST_BUILD
    s_test_config_gl33_texture_pixel_upload.call_count++;
    if(s_test_config_gl33_texture_pixel_upload.fail_on_call != 0) {
        if(s_test_config_gl33_texture_pixel_upload.call_count == s_test_config_gl33_texture_pixel_upload.fail_on_call) {
            return (renderer_result_t)s_test_config_gl33_texture_pixel_upload.forced_result;
        }
    }
#endif
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != width_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != height_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_pixel_upload", "height_")

    if(3 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width_, (GLsizei)height_, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels_);
    } else if(4 == channel_count_) {
        mock_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // 4byte境界にアラインされていないテクスチャ(width * bytes_per_pixel が 4 の倍数でないテクスチャ)に対応させるため設定
        mock_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width_, (GLsizei)height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
    } else {
        ret = RENDERER_INVALID_ARGUMENT;
        ERROR_MESSAGE("gl33_texture_pixel_upload(%s) - Provided channel count is not valid.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS == ret) {
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
#ifdef TEST_BUILD
    s_test_config_resolve_min_filter_config.call_count++;
    if(s_test_config_resolve_min_filter_config.fail_on_call != 0) {
        if(s_test_config_resolve_min_filter_config.call_count == s_test_config_resolve_min_filter_config.fail_on_call) {
            return s_test_config_resolve_min_filter_config.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_resolve_mag_filter_config.call_count++;
    if(s_test_config_resolve_mag_filter_config.fail_on_call != 0) {
        if(s_test_config_resolve_mag_filter_config.call_count == s_test_config_resolve_mag_filter_config.fail_on_call) {
            return s_test_config_resolve_mag_filter_config.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_resolve_wrap_config.call_count++;
    if(s_test_config_resolve_wrap_config.fail_on_call != 0) {
        if(s_test_config_resolve_wrap_config.call_count == s_test_config_resolve_wrap_config.fail_on_call) {
            return s_test_config_resolve_wrap_config.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_mock_glGetIntegerv.call_count++;
    if(s_test_config_mock_glGetIntegerv.fail_on_call != 0) {
        if(s_test_config_mock_glGetIntegerv.call_count >= s_test_config_mock_glGetIntegerv.fail_on_call) {
            return;
        }
    }
#endif
    glGetIntegerv(pname_, data_);
}

static void NO_COVERAGE mock_glGenTextures(GLsizei n_, GLuint* textures_) {
#ifdef TEST_BUILD
    s_test_config_mock_glGenTextures.call_count++;
    if(s_test_config_mock_glGenTextures.fail_on_call != 0) {
        if(s_test_config_mock_glGenTextures.call_count >= s_test_config_mock_glGenTextures.fail_on_call) {
            return;
        }
    }
#endif
    glGenTextures(n_, textures_);
}

static void NO_COVERAGE mock_glActiveTexture(GLenum texture_) {
#ifdef TEST_BUILD
    s_test_config_mock_glActiveTexture.call_count++;
    if(s_test_config_mock_glActiveTexture.fail_on_call != 0) {
        if(s_test_config_mock_glActiveTexture.call_count >= s_test_config_mock_glActiveTexture.fail_on_call) {
            return;
        }
    }
#endif
    glActiveTexture(texture_);
}

static void NO_COVERAGE mock_glBindTexture(GLenum target_, GLuint texture_) {
#ifdef TEST_BUILD
    s_test_config_mock_glBindTexture.call_count++;
    if(s_test_config_mock_glBindTexture.fail_on_call != 0) {
        if(s_test_config_mock_glBindTexture.call_count >= s_test_config_mock_glBindTexture.fail_on_call) {
            return;
        }
    }
#endif
    glBindTexture(target_, texture_);
}

static void NO_COVERAGE mock_glTexParameteri(GLenum target_, GLenum pname_, GLint param_) {
#ifdef TEST_BUILD
    s_test_config_mock_glTexParameteri.call_count++;
    if(s_test_config_mock_glTexParameteri.fail_on_call != 0) {
        if(s_test_config_mock_glTexParameteri.call_count >= s_test_config_mock_glTexParameteri.fail_on_call) {
            return;
        }
    }
#endif
    glTexParameteri(target_, pname_, param_);
}

static void NO_COVERAGE mock_glDeleteTextures(GLsizei n_, const GLuint* textures_) {
#ifdef TEST_BUILD
    s_test_config_mock_glDeleteTextures.call_count++;
    if(s_test_config_mock_glDeleteTextures.fail_on_call != 0) {
        if(s_test_config_mock_glDeleteTextures.call_count >= s_test_config_mock_glDeleteTextures.fail_on_call) {
            return;
        }
    }
#endif
    glDeleteTextures(n_, textures_);
}

static void NO_COVERAGE mock_glPixelStorei(GLenum pname_, GLint param_) {
#ifdef TEST_BUILD
    s_test_config_mock_glPixelStorei.call_count++;
    if(s_test_config_mock_glPixelStorei.fail_on_call != 0) {
        if(s_test_config_mock_glPixelStorei.call_count >= s_test_config_mock_glPixelStorei.fail_on_call) {
            return;
        }
    }
#endif
    glPixelStorei(pname_, param_);
}

static void NO_COVERAGE mock_glTexImage2D(GLenum target_, GLint level_, GLint internalformat_, GLsizei width_, GLsizei height_, GLint border_, GLenum format_, GLenum type_, const void* data_) {
#ifdef TEST_BUILD
    s_test_config_mock_glTexImage2D.call_count++;
    if(s_test_config_mock_glTexImage2D.fail_on_call != 0) {
        if(s_test_config_mock_glTexImage2D.call_count >= s_test_config_mock_glTexImage2D.fail_on_call) {
            return;
        }
    }
#endif
    glTexImage2D(target_, level_, internalformat_, width_, height_, border_, format_, type_, data_);
}

#ifdef TEST_BUILD
void test_concrete_texture_config_reset(void) {
    test_call_control_reset(&s_test_config_gl33_texture_create);
    test_call_control_no_op_reset(&s_test_config_gl33_texture_destroy);
    test_call_control_reset(&s_test_config_gl33_texture_bind);
    test_call_control_reset(&s_test_config_gl33_texture_unbind);
    test_call_control_reset(&s_test_config_gl33_texture_pixel_upload);
    test_call_control_bool_reset(&s_test_config_resolve_min_filter_config);
    test_call_control_bool_reset(&s_test_config_resolve_mag_filter_config);
    test_call_control_bool_reset(&s_test_config_resolve_wrap_config);
    test_call_control_no_op_reset(&s_test_config_mock_glGetIntegerv);
    test_call_control_no_op_reset(&s_test_config_mock_glGenTextures);
    test_call_control_no_op_reset(&s_test_config_mock_glActiveTexture);
    test_call_control_no_op_reset(&s_test_config_mock_glBindTexture);
    test_call_control_no_op_reset(&s_test_config_mock_glTexParameteri);
    test_call_control_no_op_reset(&s_test_config_mock_glDeleteTextures);
    test_call_control_no_op_reset(&s_test_config_mock_glPixelStorei);
    test_call_control_no_op_reset(&s_test_config_mock_glTexImage2D);
}

void test_concrete_texture(void) {
    test_gl33_texture_create();
    test_gl33_texture_destroy();
    test_gl33_texture_bind();
    test_gl33_texture_unbind();
    test_gl33_texture_pixel_upload();
    test_resolve_min_filter_config();
    test_resolve_mag_filter_config();
    test_resolve_wrap_config();
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_texture_create(void) {
    {
        // gl33_texture_create() 冒頭で強制的に RENDERER_BAD_OPERATION を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_gl33_texture_create.fail_on_call = 1U;
        s_test_config_gl33_texture_create.forced_result = (int)RENDERER_BAD_OPERATION;

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(0U == s_test_config_resolve_min_filter_config.call_count);
        assert(0U == s_test_config_resolve_mag_filter_config.call_count);
        assert(0U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            NULL
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(0U == s_test_config_resolve_min_filter_config.call_count);
        assert(0U == s_test_config_resolve_mag_filter_config.call_count);
        assert(0U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *texture_handle_ != NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t dummy = { 0 };
        renderer_backend_texture_t* texture = &dummy;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(&dummy == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(0U == s_test_config_resolve_min_filter_config.call_count);
        assert(0U == s_test_config_resolve_mag_filter_config.call_count);
        assert(0U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // resolve_min_filter_config() が false -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_min_filter_config.fail_on_call = 1U;
        s_test_config_resolve_min_filter_config.forced_result = false;

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(0U == s_test_config_resolve_mag_filter_config.call_count);
        assert(0U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // resolve_mag_filter_config() が false -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_mag_filter_config.fail_on_call = 1U;
        s_test_config_resolve_mag_filter_config.forced_result = false;

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(0U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // s軸の resolve_wrap_config() が false -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_wrap_config.fail_on_call = 1U;
        s_test_config_resolve_wrap_config.forced_result = false;

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(1U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // t軸の resolve_wrap_config() が false -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_wrap_config.fail_on_call = 2U;
        s_test_config_resolve_wrap_config.forced_result = false;

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // unit_num_ < 0 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_create(
            -1,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // メモリシステム未初期化 -> renderer_mem_allocate() 経由で RENDERER_BAD_OPERATION
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_BAD_OPERATION == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で MEMORY_SYSTEM_NO_MEMORY を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_texture_t* texture = NULL;
        test_call_control_t config = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_NO_MEMORY == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        memory_system_destroy();
        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 下位 memory_system_allocate() 冒頭で未定義値を返させる -> RENDERER_UNDEFINED_ERROR
        renderer_result_t ret = RENDERER_SUCCESS;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_texture_t* texture = NULL;
        test_call_control_t config = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        test_call_control_reset(&config);
        config.fail_on_call = 1U;
        config.forced_result = 99999;
        test_memory_system_allocate_config_set(&config);

        ret = gl33_texture_create(
            0,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_NEAREST,
            TEXTURE_WRAP_CONFIG_REPEAT,
            TEXTURE_WRAP_CONFIG_REPEAT,
            &texture
        );

        assert(RENDERER_UNDEFINED_ERROR == ret);
        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);
        assert(0U == s_test_config_mock_glGetIntegerv.call_count);
        assert(0U == s_test_config_mock_glGenTextures.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glTexParameteri.call_count);

        memory_system_destroy();
        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: texture構造体確保成功、OpenGL初期化系mockが期待回数呼ばれる
        // 実 OpenGL 呼び出しを避けるため、mock_gl*() は指定回数以降 no-op にする
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        s_test_config_mock_glGetIntegerv.fail_on_call = 1U;
        s_test_config_mock_glGenTextures.fail_on_call = 1U;
        s_test_config_mock_glActiveTexture.fail_on_call = 1U;
        s_test_config_mock_glBindTexture.fail_on_call = 1U;
        s_test_config_mock_glTexParameteri.fail_on_call = 1U;

        ret = gl33_texture_create(
            3,
            TEXTURE_MIN_FILTER_CONFIG_NEAREST,
            TEXTURE_MAG_FILTER_CONFIG_LINEAR,
            TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE,
            TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT,
            &texture
        );

        assert(RENDERER_SUCCESS == ret);
        assert(NULL != texture);

        assert(1U == s_test_config_gl33_texture_create.call_count);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);
        assert(2U == s_test_config_resolve_wrap_config.call_count);

        assert(1U == s_test_config_mock_glGetIntegerv.call_count);
        assert(1U == s_test_config_mock_glGenTextures.call_count);
        assert(2U == s_test_config_mock_glActiveTexture.call_count);
        assert(2U == s_test_config_mock_glBindTexture.call_count);
        assert(4U == s_test_config_mock_glTexParameteri.call_count);
        assert(0U == s_test_config_mock_glDeleteTextures.call_count);

        // renderer_mem_allocate() のゼロ初期化により、mock_glGenTextures() が no-op の場合は handle は 0 のまま
        assert(0U == texture->handle);
        assert(3 == texture->unit_number);
        assert(TEXTURE_MIN_FILTER_CONFIG_NEAREST == texture->min_filter_config);
        assert(TEXTURE_MAG_FILTER_CONFIG_LINEAR == texture->mag_filter_config);
        assert(TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE == texture->wrap_config_s_axis);
        assert(TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT == texture->wrap_config_t_axis);

        // 後片付け時の実 OpenGL 呼び出しを避ける
        test_concrete_texture_config_reset();
        s_test_config_mock_glBindTexture.fail_on_call = 1U;
        s_test_config_mock_glDeleteTextures.fail_on_call = 1U;

        gl33_texture_destroy(&texture);
        assert(NULL == texture);

        memory_system_destroy();
        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_texture_destroy(void) {
    {
        // gl33_texture_destroy() 冒頭で強制的に No-Op させる
        renderer_backend_texture_t dummy = { 0 };
        renderer_backend_texture_t* texture = &dummy;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        dummy.handle = 123U;
        dummy.unit_number = 0;

        s_test_config_gl33_texture_destroy.fail_on_call = 1U;

        gl33_texture_destroy(&texture);

        assert(&dummy == texture);
        assert(1U == s_test_config_gl33_texture_destroy.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glDeleteTextures.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_ == NULL の場合は No-Op
        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_texture_destroy(NULL);

        assert(1U == s_test_config_gl33_texture_destroy.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glDeleteTextures.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *texture_handle_ == NULL の場合は No-Op
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        gl33_texture_destroy(&texture);

        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_destroy.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);
        assert(0U == s_test_config_mock_glDeleteTextures.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: textureをunbindし、OpenGL texture objectを削除し、メモリ解放してNULL化する
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        memory_system_result_t ret_msys = MEMORY_SYSTEM_INVALID_ARGUMENT;
        renderer_backend_texture_t* texture = NULL;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret_msys = memory_system_create();
        assert(MEMORY_SYSTEM_SUCCESS == ret_msys);

        ret = renderer_mem_allocate(sizeof(renderer_backend_texture_t), (void**)&texture);
        assert(RENDERER_SUCCESS == ret);
        assert(NULL != texture);

        texture->handle = 456U;
        texture->unit_number = 2;
        texture->min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture->mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture->wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture->wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        test_concrete_texture_config_reset();

        s_test_config_mock_glBindTexture.fail_on_call = 1U;
        s_test_config_mock_glDeleteTextures.fail_on_call = 1U;

        gl33_texture_destroy(&texture);

        assert(NULL == texture);
        assert(1U == s_test_config_gl33_texture_destroy.call_count);
        assert(1U == s_test_config_mock_glBindTexture.call_count);
        assert(1U == s_test_config_mock_glDeleteTextures.call_count);

        memory_system_destroy();
        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_texture_bind(void) {
    {
        // gl33_texture_bind() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = -1;
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = 2;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        s_test_config_gl33_texture_bind.fail_on_call = 1U;
        s_test_config_gl33_texture_bind.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(-1 == texture_unit);
        assert(-1 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        int32_t texture_unit = -1;
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_bind(NULL, &texture_unit, &texture_internal_handle);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-1 == texture_unit);
        assert(-1 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_texture_unit_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = 2;

        ret = gl33_texture_bind(&texture, NULL, &texture_internal_handle);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-1 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_texture_internal_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = 2;

        ret = gl33_texture_bind(&texture, &texture_unit, NULL);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(-1 == texture_unit);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_->handle == 0 -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = -1;
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 0U;
        texture.unit_number = 2;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(-1 == texture_unit);
        assert(-1 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_->unit_number < 0 -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = -1;
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = -1;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(-1 == texture_unit);
        assert(-1 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 現在のbind状態と異なるため、glActiveTexture/glBindTextureを呼び出してout値を更新する
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = -1;
        int32_t texture_internal_handle = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 456U;
        texture.unit_number = 3;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        s_test_config_mock_glActiveTexture.fail_on_call = 1U;
        s_test_config_mock_glBindTexture.fail_on_call = 1U;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_SUCCESS == ret);
        assert(3 == texture_unit);
        assert(456 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(1U == s_test_config_mock_glActiveTexture.call_count);
        assert(1U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: texture unitだけ一致していてもhandleが異なればbindし直す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = 4;
        int32_t texture_internal_handle = 999;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 789U;
        texture.unit_number = 4;

        s_test_config_mock_glActiveTexture.fail_on_call = 1U;
        s_test_config_mock_glBindTexture.fail_on_call = 1U;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_SUCCESS == ret);
        assert(4 == texture_unit);
        assert(789 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(1U == s_test_config_mock_glActiveTexture.call_count);
        assert(1U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: handleだけ一致していてもtexture unitが異なればbindし直す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = 1;
        int32_t texture_internal_handle = 321;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 321U;
        texture.unit_number = 5;

        s_test_config_mock_glActiveTexture.fail_on_call = 1U;
        s_test_config_mock_glBindTexture.fail_on_call = 1U;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_SUCCESS == ret);
        assert(5 == texture_unit);
        assert(321 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(1U == s_test_config_mock_glActiveTexture.call_count);
        assert(1U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: すでに同じtexture unit / handleがbind済みならOpenGL呼び出しを省略する
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };
        int32_t texture_unit = 6;
        int32_t texture_internal_handle = 654;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 654U;
        texture.unit_number = 6;

        ret = gl33_texture_bind(&texture, &texture_unit, &texture_internal_handle);

        assert(RENDERER_SUCCESS == ret);
        assert(6 == texture_unit);
        assert(654 == texture_internal_handle);
        assert(1U == s_test_config_gl33_texture_bind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_texture_unbind(void) {
    {
        // gl33_texture_unbind() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = 2;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        s_test_config_gl33_texture_unbind.fail_on_call = 1U;
        s_test_config_gl33_texture_unbind.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_texture_unbind(&texture);

        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_gl33_texture_unbind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_unbind(NULL);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_unbind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_->handle == 0 -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 0U;
        texture.unit_number = 2;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        ret = gl33_texture_unbind(&texture);

        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(1U == s_test_config_gl33_texture_unbind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // texture_handle_->unit_number < 0 -> RENDERER_DATA_CORRUPTED
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 123U;
        texture.unit_number = -1;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        ret = gl33_texture_unbind(&texture);

        assert(RENDERER_DATA_CORRUPTED == ret);
        assert(1U == s_test_config_gl33_texture_unbind.call_count);
        assert(0U == s_test_config_mock_glActiveTexture.call_count);
        assert(0U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 対象texture unitをactiveにしてからGL_TEXTURE_2Dをunbindする
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        renderer_backend_texture_t texture = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        texture.handle = 456U;
        texture.unit_number = 3;
        texture.min_filter_config = TEXTURE_MIN_FILTER_CONFIG_NEAREST;
        texture.mag_filter_config = TEXTURE_MAG_FILTER_CONFIG_LINEAR;
        texture.wrap_config_s_axis = TEXTURE_WRAP_CONFIG_REPEAT;
        texture.wrap_config_t_axis = TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE;

        s_test_config_mock_glActiveTexture.fail_on_call = 1U;
        s_test_config_mock_glBindTexture.fail_on_call = 1U;

        ret = gl33_texture_unbind(&texture);

        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_gl33_texture_unbind.call_count);
        assert(1U == s_test_config_mock_glActiveTexture.call_count);
        assert(1U == s_test_config_mock_glBindTexture.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_gl33_texture_pixel_upload(void) {
    {
        // gl33_texture_pixel_upload() 冒頭で強制的に RENDERER_RUNTIME_ERROR を返させる
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_gl33_texture_pixel_upload.fail_on_call = 1U;
        s_test_config_gl33_texture_pixel_upload.forced_result = (int)RENDERER_RUNTIME_ERROR;

        ret = gl33_texture_pixel_upload(2U, 2U, 3U, pixels);

        assert(RENDERER_RUNTIME_ERROR == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // pixels_ == NULL -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 2U, 3U, NULL);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // width_ == 0 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(0U, 2U, 3U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // height_ == 0 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 0U, 3U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // channel_count_ == 0 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 2U, 0U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // channel_count_ == 1 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 2U, 1U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // channel_count_ == 2 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 2U, 2U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // channel_count_ == 5 -> RENDERER_INVALID_ARGUMENT
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[20] = { 0 };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = gl33_texture_pixel_upload(2U, 2U, 5U, pixels);

        assert(RENDERER_INVALID_ARGUMENT == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(0U == s_test_config_mock_glPixelStorei.call_count);
        assert(0U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: RGB画像をuploadし、GL_UNPACK_ALIGNMENTを1に変更してから4へ戻す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[12] = {
            255U, 0U,   0U,
            0U,   255U, 0U,
            0U,   0U,   255U,
            255U, 255U, 255U
        };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_mock_glPixelStorei.fail_on_call = 1U;
        s_test_config_mock_glTexImage2D.fail_on_call = 1U;

        ret = gl33_texture_pixel_upload(2U, 2U, 3U, pixels);

        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(2U == s_test_config_mock_glPixelStorei.call_count);
        assert(1U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: RGBA画像をuploadし、GL_UNPACK_ALIGNMENTを1に変更してから4へ戻す
        renderer_result_t ret = RENDERER_UNDEFINED_ERROR;
        uint8_t pixels[16] = {
            255U, 0U,   0U,   255U,
            0U,   255U, 0U,   255U,
            0U,   0U,   255U, 255U,
            255U, 255U, 255U, 255U
        };

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_mock_glPixelStorei.fail_on_call = 1U;
        s_test_config_mock_glTexImage2D.fail_on_call = 1U;

        ret = gl33_texture_pixel_upload(2U, 2U, 4U, pixels);

        assert(RENDERER_SUCCESS == ret);
        assert(1U == s_test_config_gl33_texture_pixel_upload.call_count);
        assert(2U == s_test_config_mock_glPixelStorei.call_count);
        assert(1U == s_test_config_mock_glTexImage2D.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_resolve_min_filter_config(void) {
    {
        // resolve_min_filter_config() 冒頭で強制的に false を返させる
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_min_filter_config.fail_on_call = 1U;
        s_test_config_resolve_min_filter_config.forced_result = false;

        ret = resolve_min_filter_config(TEXTURE_MIN_FILTER_CONFIG_NEAREST, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // dst_ == NULL -> false
        bool ret = true;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_min_filter_config(TEXTURE_MIN_FILTER_CONFIG_NEAREST, NULL);

        assert(false == ret);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_MIN_FILTER_CONFIG_NEAREST -> GL_NEAREST
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_min_filter_config(TEXTURE_MIN_FILTER_CONFIG_NEAREST, &dst);

        assert(true == ret);
        assert(GL_NEAREST == dst);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_MIN_FILTER_CONFIG_LINEAR -> GL_LINEAR
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_min_filter_config(TEXTURE_MIN_FILTER_CONFIG_LINEAR, &dst);

        assert(true == ret);
        assert(GL_LINEAR == dst);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未定義のmin filter config -> false
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_min_filter_config((texture_min_filter_config_t)99999, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_min_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_resolve_mag_filter_config(void) {
    {
        // resolve_mag_filter_config() 冒頭で強制的に false を返させる
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_mag_filter_config.fail_on_call = 1U;
        s_test_config_resolve_mag_filter_config.forced_result = false;

        ret = resolve_mag_filter_config(TEXTURE_MAG_FILTER_CONFIG_NEAREST, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // dst_ == NULL -> false
        bool ret = true;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_mag_filter_config(TEXTURE_MAG_FILTER_CONFIG_NEAREST, NULL);

        assert(false == ret);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_MAG_FILTER_CONFIG_NEAREST -> GL_NEAREST
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_mag_filter_config(TEXTURE_MAG_FILTER_CONFIG_NEAREST, &dst);

        assert(true == ret);
        assert(GL_NEAREST == dst);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_MAG_FILTER_CONFIG_LINEAR -> GL_LINEAR
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_mag_filter_config(TEXTURE_MAG_FILTER_CONFIG_LINEAR, &dst);

        assert(true == ret);
        assert(GL_LINEAR == dst);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未定義のmag filter config -> false
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_mag_filter_config((texture_mag_filter_config_t)99999, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_mag_filter_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_resolve_wrap_config(void) {
    {
        // resolve_wrap_config() 冒頭で強制的に false を返させる
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        s_test_config_resolve_wrap_config.fail_on_call = 1U;
        s_test_config_resolve_wrap_config.forced_result = false;

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_REPEAT, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // dst_ == NULL -> false
        bool ret = true;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_REPEAT, NULL);

        assert(false == ret);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_WRAP_CONFIG_REPEAT -> GL_REPEAT
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_REPEAT, &dst);

        assert(true == ret);
        assert(GL_REPEAT == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE -> GL_CLAMP_TO_EDGE
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_CLAMP_TO_EDGE, &dst);

        assert(true == ret);
        assert(GL_CLAMP_TO_EDGE == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT -> GL_MIRRORED_REPEAT
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_MIRRORED_REPEAT, &dst);

        assert(true == ret);
        assert(GL_MIRRORED_REPEAT == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未定義のwrap config -> false
        bool ret = true;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config((texture_wrap_config_t)99999, &dst);

        assert(false == ret);
        assert(-1 == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // TEXTURE_WRAP_CONFIG_CLAMP_TO_BORDER -> GL_CLAMP_TO_BORDER
        bool ret = false;
        GLint dst = -1;

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
        memory_system_destroy();

        ret = resolve_wrap_config(TEXTURE_WRAP_CONFIG_CLAMP_TO_BORDER, &dst);

        assert(true == ret);
        assert(GL_CLAMP_TO_BORDER == dst);
        assert(1U == s_test_config_resolve_wrap_config.call_count);

        test_concrete_texture_config_reset();
        test_choco_memory_config_reset();
    }
}

static void NO_COVERAGE test_call_control_no_op_reset(test_call_control_no_op_t* config_) {
    config_->call_count = 0;
    config_->fail_on_call = 0;
}
#endif
