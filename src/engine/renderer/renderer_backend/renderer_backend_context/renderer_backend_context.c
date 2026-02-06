#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/renderer/renderer_core/renderer_types.h"
#include "engine/renderer/renderer_core/renderer_err_utils.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/renderer_backend_shader_context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/renderer_backend_vao_context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/renderer_backend_vbo_context.h"

#include "engine/renderer/renderer_backend/renderer_backend_interface/shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/vertex_array_object.h"
#include "engine/renderer/renderer_backend/renderer_backend_interface/vertex_buffer_object.h"

#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_shader.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vao.h"
#include "engine/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vbo.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

struct renderer_backend_context {
    target_graphics_api_t target_api;

    const renderer_shader_vtable_t* shader_vtable;
    const renderer_vao_vtable_t* vao_vtable;
    const renderer_vbo_vtable_t* vbo_vtable;

    uint32_t current_program_id;
    uint32_t current_bound_vao;
    uint32_t current_bound_vbo;
};

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_);
static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_);
static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_);

static bool graphics_api_valid_check(target_graphics_api_t target_api_);

renderer_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "out_renderer_backend_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "*out_renderer_backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(graphics_api_valid_check(target_api_), RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_initialize", "target_api_")

    // Simulation.
    renderer_backend_context_t* tmp_context = NULL;
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(renderer_backend_context_t), alignof(renderer_backend_context_t), (void**)&tmp_context);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = renderer_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to allocate memory for renderer backend context.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_context, 0, sizeof(renderer_backend_context_t));

    // shaderバックエンドメモリ確保+初期化
    tmp_context->shader_vtable = shader_vtable_get(target_api_);
    if(NULL == tmp_context->shader_vtable) {
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get shader vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // vaoバックエンドメモリ確保+初期化
    tmp_context->vao_vtable = vao_vtable_get(target_api_);
    if(NULL == tmp_context->vao_vtable) {
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vao vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // vboバックエンドメモリ確保+初期化
    tmp_context->vbo_vtable = vbo_vtable_get(target_api_);
    if(NULL == tmp_context->vbo_vtable) {
        ret = RENDERER_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vbo vtable.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->target_api = target_api_;
    tmp_context->current_bound_vao = 0;
    tmp_context->current_bound_vbo = 0;
    tmp_context->current_program_id = 0;

    // commit.
    *out_renderer_backend_context_ = tmp_context;
    ret = RENDERER_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void renderer_backend_destroy(renderer_backend_context_t* renderer_context_) {
    if(NULL == renderer_context_) {
        goto cleanup;
    }
    // 現状では特に必要な処理はなし
cleanup:
    return;
}

renderer_result_t renderer_backend_shader_create(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_create", "renderer_backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_create", "*shader_handle_")

    ret = renderer_backend_context_->shader_vtable->renderer_shader_create(shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_create(%s) - Failed to create shader handle.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_shader_destroy(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    if(NULL == renderer_backend_context_ || NULL == renderer_backend_context_->shader_vtable) {
        return;
    }
    renderer_backend_context_->shader_vtable->renderer_shader_destroy(shader_handle_);
}

renderer_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_compile", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_compile(shader_type_, shader_source_, shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_compile(%s) - Failed to compile shader source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_link", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_link", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_link", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_link(shader_handle_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_link(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_shader_use(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_shader_use", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_shader_use", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_use(shader_handle_, &backend_context_->current_program_id);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_use(%s) - Failed to use shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_create", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "vertex_array_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "*vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_create(vertex_array_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_create(%s) - Failed to create vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return ret;
}

void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    if(NULL == backend_context_ || NULL == backend_context_->vao_vtable) {
        return;
    }
    backend_context_->vao_vtable->vertex_array_destroy(vertex_array_);
}

renderer_result_t renderer_backend_vertex_array_bind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_bind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_bind(vertex_array_, &backend_context_->current_bound_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_bind(%s) - Failed to bind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_unbind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_unbind", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_unbind(vertex_array_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_unbind(%s) - Failed to unbind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    backend_context_->current_bound_vao = 0;

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_array_attribute_set(renderer_backend_context_t* backend_context_, renderer_backend_vao_t* vertex_array_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_attribute_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_array_attribute_set", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_array_attribute_set", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_bind(vertex_array_, &backend_context_->current_bound_vao);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_attribute_set(%s) - Failed to bind vao.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->vao_vtable->vertex_array_attribute_set(vertex_array_, layout_, size_, type_, normalized_, stride_, offset_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_attribute_set(%s) - Failed to set vao attribute.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_create", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "vertex_buffer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "*vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_create(vertex_buffer_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_create(%s) - Failed to create vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
cleanup:
    return ret;
}

void renderer_backend_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    if(NULL == backend_context_ || NULL == backend_context_->vbo_vtable) {
        return;
    }
    backend_context_->vbo_vtable->vertex_buffer_destroy(vertex_buffer_);
}

renderer_result_t renderer_backend_vertex_buffer_bind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_bind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_, &backend_context_->current_bound_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_bind(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_unbind(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_unbind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_unbind", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_unbind(vertex_buffer_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_unbind(%s) - Failed to unbind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    backend_context_->current_bound_vbo = 0;

cleanup:
    return ret;
}

renderer_result_t renderer_backend_vertex_buffer_vertex_load(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t* vertex_buffer_, size_t load_size_, void* load_data_, buffer_usage_t usage_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_load", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BAD_OPERATION, renderer_rslt_to_str(RENDERER_BAD_OPERATION), "renderer_backend_vertex_buffer_vertex_load", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_load", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_, &backend_context_->current_bound_vbo);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_load(%s) - Failed to bind vbo.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = backend_context_->vbo_vtable->vertex_buffer_vertex_load(vertex_buffer_, load_size_, load_data_, usage_);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_load(%s) - Failed to load vertex.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_) {
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_shader_vtable_get();
    default:
        return NULL;
    }
}

static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_) {
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_vao_vtable_get();
    default:
        return NULL;
    }
}

static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_) {
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_vbo_vtable_get();
    default:
        return NULL;
    }
}

static bool graphics_api_valid_check(target_graphics_api_t target_api_) {
    bool ret = false;
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}
