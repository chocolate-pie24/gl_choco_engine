#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <GL/glew.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_texture.h"

#include "engine/renderer/renderer_core/renderer_memory.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_types.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#ifdef TEST_BUILD
#include <assert.h>
#endif

struct renderer_backend_texture {
    // TODO: 型はOpenGLのAPI仕様を見ながら合わせる
    int32_t width;
    int32_t height;
    GLuint handle;
    uint8_t channel_count;
    int32_t unit_number;

    texture_min_filter_config_t min_filter_config;
    texture_mag_filter_config_t mag_filter_config;
    texture_wrap_config_t wrap_config_s_axis;
    texture_wrap_config_t wrap_config_t_axis;
};

static renderer_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_);
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_);
static renderer_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_internal_handle_);
static renderer_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_);
static renderer_result_t gl33_texture_pixel_upload(const uint8_t* pixels_, const renderer_backend_texture_t* texture_handle_);

static bool resolv_min_filter_config(texture_min_filter_config_t src_, GLint* dst_);
static bool resolv_mag_filter_config(texture_mag_filter_config_t src_, GLint* dst_);
static bool resolv_wrap_config(texture_wrap_config_t src_, GLint* dst_);

static const renderer_texture_vtable_t s_gl33_texture_vtable = {
    .renderer_texture_create = gl33_texture_create,
    .renderer_texture_destroy = gl33_texture_destroy,
    .renderer_texture_bind = gl33_texture_bind,
    .renderer_texture_unbind = gl33_texture_unbind,
    .renderer_texture_pixel_upload = gl33_texture_pixel_upload,
};

const renderer_texture_vtable_t* gl33_texture_vtable_get(void) {
    // TODO: 外部からの失敗注入についてどうするか考える
    return &s_gl33_texture_vtable;
}

static renderer_result_t gl33_texture_create(int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    renderer_backend_texture_t* tmp = NULL;
    GLint min_filter = GL_NEAREST;
    GLint mag_filter = GL_NEAREST;
    GLint wrap_config_s_axis = GL_REPEAT;
    GLint wrap_config_t_axis = GL_REPEAT;
    GLint current_unit = GL_TEXTURE0;

    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "*texture_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolv_min_filter_config(min_filter_config_, &min_filter), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "min_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolv_mag_filter_config(mag_filter_config_, &mag_filter), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "mag_filter_config_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolv_wrap_config(wrap_config_s_axis_, &wrap_config_s_axis), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_s_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(resolv_wrap_config(wrap_config_t_axis_, &wrap_config_t_axis), ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "wrap_config_t_axis_")
    IF_ARG_FALSE_GOTO_CLEANUP(unit_num_ >= 0, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "gl33_texture_create", "unit_num_")

    ret = renderer_mem_allocate(sizeof(renderer_backend_texture_t), (void**)&tmp);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("gl33_texture_create(%s) - Failed to allocate memory for texture handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    tmp->width = 0;
    tmp->height = 0;
    tmp->handle = 0;
    tmp->channel_count = 0;
    tmp->unit_number = unit_num_;
    tmp->min_filter_config = min_filter_config_;
    tmp->mag_filter_config = mag_filter_config_;
    tmp->wrap_config_s_axis = wrap_config_s_axis_;
    tmp->wrap_config_t_axis = wrap_config_t_axis_;

    glGetIntegerv(GL_ACTIVE_TEXTURE, &current_unit);

    glGenTextures(1, &tmp->handle);
    glActiveTexture(GL_TEXTURE0 + (GLenum)unit_num_);

    glBindTexture(GL_TEXTURE_2D, tmp->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_config_s_axis);   // s軸
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_config_t_axis);   // t軸

    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture((GLenum)current_unit);

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

static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_) {

}

static renderer_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_internal_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    return ret;
}

static renderer_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    return ret;
}

static renderer_result_t gl33_texture_pixel_upload(const uint8_t* pixels_, const renderer_backend_texture_t* texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

    return ret;
}

static bool resolv_min_filter_config(texture_min_filter_config_t src_, GLint* dst_) {
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
    case TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_NEAREST:
        *dst_ = GL_NEAREST_MIPMAP_NEAREST;
        ret = true;
        break;
    case TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_NEAREST:
        *dst_ = GL_LINEAR_MIPMAP_NEAREST;
        ret = true;
        break;
    case TEXTURE_MIN_FILTER_CONFIG_NEAREST_MIPMAP_LINEAR:
        *dst_ = GL_NEAREST_MIPMAP_LINEAR;
        ret = true;
        break;
    case TEXTURE_MIN_FILTER_CONFIG_LINEAR_MIPMAP_LINEAR:
        *dst_ = GL_LINEAR_MIPMAP_LINEAR;
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

static bool resolv_mag_filter_config(texture_mag_filter_config_t src_, GLint* dst_) {
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

static bool resolv_wrap_config(texture_wrap_config_t src_, GLint* dst_) {
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
