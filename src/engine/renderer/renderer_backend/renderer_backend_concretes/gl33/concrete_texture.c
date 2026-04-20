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

struct renderer_backend_texture {
    // TODO: 型はOpenGLのAPI仕様を見ながら合わせる
    int32_t width;
    int32_t height;
    int32_t handle;
    uint8_t channel_count;
    int32_t unit_number;
};

static renderer_result_t gl33_texture_create(int32_t unit_num_, renderer_backend_texture_t** texture_handle_);
static void gl33_texture_destroy(renderer_backend_texture_t** texture_handle_);
static renderer_result_t gl33_texture_bind(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_internal_handle_);
static renderer_result_t gl33_texture_unbind(const renderer_backend_texture_t* texture_handle_);
static renderer_result_t gl33_texture_pixel_upload(const uint8_t* pixels_, const renderer_backend_texture_t* texture_handle_);

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

static renderer_result_t gl33_texture_create(int32_t unit_num_, renderer_backend_texture_t** texture_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;

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
