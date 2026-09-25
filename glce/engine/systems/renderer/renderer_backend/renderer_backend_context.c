// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/internal/renderer_backend_context_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_err_utils.h"

#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_shader_vtable.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vao_vtable.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_vbo_vtable.h"
#include "engine/systems/renderer/renderer_backend/vtables/renderer_backend_texture_vtable.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_vbo.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_concretes/gl33/gl33_texture.h"

#include "config/build_config.h"

static const renderer_shader_vtable_t* shader_vtable_get(void);
static const renderer_vao_vtable_t* vao_vtable_get(void);
static const renderer_vbo_vtable_t* vbo_vtable_get(void);
static const renderer_texture_vtable_t* texture_vtable_get(void);

renderer_backend_result_t renderer_backend_create(subsystem_allocator_t* allocator_, renderer_backend_context_t** out_backend_context_) {
    renderer_backend_result_t ret = RENDERER_BACKEND_INVALID_ARGUMENT;

    subsystem_allocator_result_t ret_subsystem_allocator = SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT;

    renderer_backend_context_t* tmp_backend_context = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_create", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_create", "out_backend_context_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_backend_context_, ret, RENDERER_BACKEND_INVALID_ARGUMENT, renderer_backend_result_to_str(RENDERER_BACKEND_INVALID_ARGUMENT), "renderer_backend_create", "*out_backend_context_")

    // Simulation.
    ret_subsystem_allocator = subsystem_allocator_allocate(allocator_, sizeof(renderer_backend_context_t), SUBSYSTEM_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_backend_context);
    if(SUBSYSTEM_ALLOCATOR_SUCCESS != ret_subsystem_allocator) {
        ret = renderer_backend_result_convert_subsystem_allocator(ret_subsystem_allocator);
        ERROR_MESSAGE("renderer_backend_create(%s) - subsystem_allocator_allocate failed.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }
    memset(tmp_backend_context, 0, sizeof(renderer_backend_context_t));

    // shaderバックエンドメモリ確保+初期化
    tmp_backend_context->shader_vtable = shader_vtable_get();
    if(NULL == tmp_backend_context->shader_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_create(%s) - Failed to get shader vtable.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // vaoバックエンドメモリ確保+初期化
    tmp_backend_context->vao_vtable = vao_vtable_get();
    if(NULL == tmp_backend_context->vao_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_create(%s) - Failed to get vao vtable.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // vboバックエンドメモリ確保+初期化
    tmp_backend_context->vbo_vtable = vbo_vtable_get();
    if(NULL == tmp_backend_context->vbo_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_create(%s) - Failed to get vbo vtable.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // Textureバックエンドメモリ確保+初期化
    tmp_backend_context->texture_vtable = texture_vtable_get();
    if(NULL == tmp_backend_context->texture_vtable) {
        // ここは引数チェックが事前にされているので通らないため、カバレッジは100にならないが、許容
        ret = RENDERER_BACKEND_RUNTIME_ERROR;
        ERROR_MESSAGE("renderer_backend_create(%s) - Failed to get texture vtable.", renderer_backend_result_to_str(ret));
        goto cleanup;
    }

    // commit.
    *out_backend_context_ = tmp_backend_context;

    ret = RENDERER_BACKEND_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void renderer_backend_deinitialize(renderer_backend_context_t* backend_context_) {
    if(NULL == backend_context_) {
        goto cleanup;
    }
    // 現状では特に必要な処理はなし(リニアロケータによるメモリ確保のため、backend_context_のリソース解放は不要)
cleanup:
    return;
}

static const renderer_shader_vtable_t* shader_vtable_get(void) {
#if defined(GLCE_BUILD_GRAPHICS_API_GL33)
    return gl33_shader_vtable_get();
#else
    return NULL;
#endif
}

static const renderer_vao_vtable_t* vao_vtable_get(void) {
#if defined(GLCE_BUILD_GRAPHICS_API_GL33)
    return gl33_vao_vtable_get();
#else
    return NULL;
#endif
}

static const renderer_vbo_vtable_t* vbo_vtable_get(void) {
#if defined(GLCE_BUILD_GRAPHICS_API_GL33)
    return gl33_vbo_vtable_get();
#else
    return NULL;
#endif
}

static const renderer_texture_vtable_t* texture_vtable_get(void) {
#if defined(GLCE_BUILD_GRAPHICS_API_GL33)
    return gl33_texture_vtable_get();
#else
    return NULL;
#endif
}
