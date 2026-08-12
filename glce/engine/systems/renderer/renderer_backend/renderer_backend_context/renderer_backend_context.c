#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/internal/renderer_backend_context_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_texture.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_interface/interface_texture.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/concrete_texture.h"

static const renderer_shader_vtable_t* shader_vtable_get(target_graphics_api_t target_api_);
static const renderer_vao_vtable_t* vao_vtable_get(target_graphics_api_t target_api_);
static const renderer_vbo_vtable_t* vbo_vtable_get(target_graphics_api_t target_api_);
static const renderer_texture_vtable_t* texture_vtable_get(target_graphics_api_t target_api_);

static bool graphics_api_valid_check(target_graphics_api_t target_api_);

renderer_backend_result_t renderer_backend_initialize(linear_alloc_t* allocator_, target_graphics_api_t target_api_, renderer_backend_context_t** out_renderer_backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;
    renderer_backend_context_t* tmp_context = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_renderer_backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_initialize", "out_renderer_backend_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_renderer_backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_initialize", "*out_renderer_backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(graphics_api_valid_check(target_api_), ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_initialize", "target_api_")

    // Simulation.
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(renderer_backend_context_t), alignof(renderer_backend_context_t), (void**)&tmp_context);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = renderer_backend_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to allocate memory for renderer backend context.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_context, 0, sizeof(renderer_backend_context_t));

    // shaderバックエンドメモリ確保+初期化
    tmp_context->shader_vtable = shader_vtable_get(target_api_);
    if(NULL == tmp_context->shader_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get shader vtable.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // vaoバックエンドメモリ確保+初期化
    tmp_context->vao_vtable = vao_vtable_get(target_api_);
    if(NULL == tmp_context->vao_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vao vtable.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // vboバックエンドメモリ確保+初期化
    tmp_context->vbo_vtable = vbo_vtable_get(target_api_);
    if(NULL == tmp_context->vbo_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get vbo vtable.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    // Textureバックエンドメモリ確保+初期化
    tmp_context->texture_vtable = texture_vtable_get(target_api_);
    if(NULL == tmp_context->texture_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_initialize(%s) - Failed to get texture vtable.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_context->target_api = target_api_;

    // commit.
    *out_renderer_backend_context_ = tmp_context;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void renderer_backend_destroy(renderer_backend_context_t* renderer_context_) {
    if(NULL == renderer_context_) {
        goto cleanup;
    }
    // 現状では特に必要な処理はなし(リニアロケータによるメモリ確保のため、renderer_context_のリソース解放は不要)
cleanup:
    return;
}

renderer_backend_result_t renderer_backend_shader_create(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_create", "renderer_backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_create", "shader_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_create", "*shader_handle_")

    ret = renderer_backend_context_->shader_vtable->renderer_shader_create(shader_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_create(%s) - Failed to create shader handle.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_shader_destroy(renderer_backend_context_t* renderer_backend_context_, renderer_backend_shader_t** shader_handle_) {
    if(NULL == renderer_backend_context_ || NULL == renderer_backend_context_->shader_vtable) {
        return;
    }
    // NOTE: shader_handle_のNULLチェックは下位に任せる
    renderer_backend_context_->shader_vtable->renderer_shader_destroy(shader_handle_);
}

renderer_backend_result_t renderer_backend_shader_compile(shader_type_t shader_type_, const char* shader_source_, renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_compile", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_source_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_compile", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_compile", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_compile(shader_type_, shader_source_, shader_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_compile(%s) - Failed to compile shader source.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_shader_link(renderer_backend_context_t* backend_context_, renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_link", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_link", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_link", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_link(shader_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_link(%s) - Failed to link shader program.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_shader_use(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_use", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_use", "shader_handle_")

    ret = backend_context_->shader_vtable->renderer_shader_use(shader_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_use(%s) - Failed to use shader program.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_shader_uniform_location_get(const renderer_backend_context_t* backend_context_, const renderer_backend_shader_t* shader_handle_, const char* name_, int32_t* out_location_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_uniform_location_get", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(shader_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "shader_handle_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_location_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_uniform_location_get", "out_location_")

    ret = backend_context_->shader_vtable->renderer_shader_uniform_location_get(shader_handle_, name_, out_location_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_uniform_location_get(%s) - Failed to get uniform location. name: %s", renderer_backend_rslt_to_str(ret), name_);
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_shader_mat4f_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, bool should_transpose_, const float* data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_mat4f_uniform_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_mat4f_uniform_set", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_mat4f_uniform_set", "data_")

    ret = backend_context_->shader_vtable->renderer_shader_mat4f_uniform_set(location_, should_transpose_, data_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_mat4f_uniform_set(%s) - Failed to set mat4f uniform.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_shader_vec4u8_uniform_set(const renderer_backend_context_t* backend_context_, int32_t location_, const uint8_t* data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_vec4u8_uniform_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->shader_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_shader_vec4u8_uniform_set", "backend_context_->shader_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_shader_vec4u8_uniform_set", "data_")

    ret = backend_context_->shader_vtable->renderer_shader_vec4u8_uniform_set(location_, data_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_shader_vec4u8_uniform_set(%s) - Failed to set vec4u8 uniform.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_array_create(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_array_create", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "vertex_array_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_array_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_create", "*vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_create(vertex_array_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_create(%s) - Failed to create vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_vertex_array_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vao_t** vertex_array_) {
    if(NULL == backend_context_ || NULL == backend_context_->vao_vtable) {
        return;
    }
    // NOTE: vertex_array_のNULLチェックは下位で行う
    backend_context_->vao_vtable->vertex_array_destroy(vertex_array_);
}

renderer_backend_result_t renderer_backend_vertex_array_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vertex_array_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_array_bind", "backend_context_->vao_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_array_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_bind", "vertex_array_")

    ret = backend_context_->vao_vtable->vertex_array_bind(vertex_array_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_bind(%s) - Failed to bind vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_array_unbind(const renderer_backend_context_t* backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_array_unbind", "backend_context_->vao_vtable")

    ret = backend_context_->vao_vtable->vertex_array_unbind();
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_unbind(%s) - Failed to unbind vao.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_array_attribute_set(const renderer_backend_context_t* backend_context_, uint32_t layout_, int32_t size_, renderer_type_t type_, bool normalized_, size_t stride_, size_t offset_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_array_attribute_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vao_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_array_attribute_set", "backend_context_->vao_vtable")

    ret = backend_context_->vao_vtable->vertex_array_attribute_set(layout_, size_, type_, normalized_, stride_, offset_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_array_attribute_set(%s) - Failed to set vao attribute.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_buffer_create(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_buffer_create", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "vertex_buffer_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*vertex_buffer_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_create", "*vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_create(vertex_buffer_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_create(%s) - Failed to create vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_vertex_buffer_destroy(renderer_backend_context_t* backend_context_, renderer_backend_vbo_t** vertex_buffer_) {
    if(NULL == backend_context_ || NULL == backend_context_->vbo_vtable) {
        return;
    }
    // NOTE: vertex_buffer_のNULLチェックは下位に任せる
    backend_context_->vbo_vtable->vertex_buffer_destroy(vertex_buffer_);
}

renderer_backend_result_t renderer_backend_vertex_buffer_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vertex_buffer_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_buffer_bind", "backend_context_->vbo_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_buffer_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_bind", "vertex_buffer_")

    ret = backend_context_->vbo_vtable->vertex_buffer_bind(vertex_buffer_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_bind(%s) - Failed to bind vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_buffer_unbind(const renderer_backend_context_t* backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_buffer_unbind", "backend_context_->vbo_vtable")

    ret = backend_context_->vbo_vtable->vertex_buffer_unbind();
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_unbind(%s) - Failed to unbind vbo.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_buffer_vertex_load(const renderer_backend_context_t* backend_context_, size_t load_size_, const void* load_data_, buffer_usage_t usage_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_load", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_buffer_vertex_load", "backend_context_->vbo_vtable")

    ret = backend_context_->vbo_vtable->vertex_buffer_vertex_load(load_size_, load_data_, usage_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_load(%s) - Failed to load vertex.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_vertex_buffer_vertex_subload(const renderer_backend_context_t* backend_context_, size_t offset_, size_t size_, const void* load_data_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_subload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->vbo_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_vertex_buffer_vertex_subload", "backend_context_->vbo_vtable")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_subload", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(load_data_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_vertex_buffer_vertex_subload", "load_data_")

    ret = backend_context_->vbo_vtable->vertex_buffer_vertex_subload(offset_, size_, load_data_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_vertex_buffer_vertex_subload(%s) - Failed to load vertex.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_texture_create(renderer_backend_context_t* backend_context_, int32_t unit_num_, texture_min_filter_config_t min_filter_config_, texture_mag_filter_config_t mag_filter_config_, texture_wrap_config_t wrap_config_s_axis_, texture_wrap_config_t wrap_config_t_axis_, renderer_backend_texture_t** texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_create", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "texture_handle_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_create", "*texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_create(unit_num_, min_filter_config_, mag_filter_config_, wrap_config_s_axis_, wrap_config_t_axis_, texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_create(%s) - Failed to create texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

void renderer_backend_texture_destroy(renderer_backend_context_t* backend_context_, renderer_backend_texture_t** texture_handle_) {
    if(NULL == backend_context_ || NULL == backend_context_->texture_vtable) {
        WARN_MESSAGE("renderer_backend_texture_destroy - Provided backend_context_ or backend_context_->texture_vtable is not valid.");
        return;
    }
    if(NULL == texture_handle_ || NULL == *texture_handle_) {
        WARN_MESSAGE("renderer_backend_texture_destroy - Provided texture_handle_ or *texture_handle_ is not valid.");
        return;
    }
    backend_context_->texture_vtable->renderer_texture_destroy(texture_handle_);
}

renderer_backend_result_t renderer_backend_texture_bind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_bind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_bind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_bind(texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_bind(%s) - Failed to bind texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_texture_unbind(const renderer_backend_context_t* backend_context_, const renderer_backend_texture_t* texture_handle_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_unbind", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(texture_handle_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_unbind", "texture_handle_")

    ret = backend_context_->texture_vtable->renderer_texture_unbind(texture_handle_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_unbind(%s) - Failed to unbind texture.", renderer_backend_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}

renderer_backend_result_t renderer_backend_texture_pixel_upload(const renderer_backend_context_t* backend_context_, uint32_t width_, uint32_t height_, uint8_t channel_count_, const uint8_t* pixels_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_->texture_vtable, ret, RENDERER_BACKEND_BAD_OPERATION, renderer_backend_rslt_to_str(RENDERER_BACKEND_BAD_OPERATION), "renderer_backend_texture_pixel_upload", "backend_context_->texture_vtable")
    IF_ARG_NULL_GOTO_CLEANUP(pixels_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "pixels_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != width_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "width_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != height_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_rslt_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_texture_pixel_upload", "height_")

    if(3 != channel_count_ && 4 != channel_count_) {
        ret = RENDERER_BACKEND_INVALID_ARGUMENT;
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Invalid channel_count_. expected = 3(RGB) or 4(RGBA), actual = %d", renderer_backend_rslt_to_str(ret), channel_count_);
        goto cleanup;
    }

    ret = backend_context_->texture_vtable->renderer_texture_pixel_upload(width_, height_, channel_count_, pixels_);
    if(RENDERER_BACKEND_SUCCESS != ret) {
        ERROR_MESSAGE("renderer_backend_texture_pixel_upload(%s) - Failed to upload texture pixels.", renderer_backend_rslt_to_str(ret));
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

static const renderer_texture_vtable_t* texture_vtable_get(target_graphics_api_t target_api_) {
    switch(target_api_) {
    case GRAPHICS_API_GL33:
        return gl33_texture_vtable_get();
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
