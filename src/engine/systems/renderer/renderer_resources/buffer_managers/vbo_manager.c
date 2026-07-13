#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_core/allocators/range_free_list.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_vbo.h"

#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_types.h"
#include "engine/systems/renderer/renderer_resources/buffer_managers/buffer_manager_err_utils.h"

struct vbo_manager {
    vbo_manager_config_t config;
    range_free_list_t* range_free_list;
    renderer_backend_vbo_t* vbo;
};

buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_free_list_result_t ret_allocator = RANGE_FREE_LIST_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;
    range_free_list_t* tmp_allocator = NULL;
    renderer_backend_vbo_t* tmp_vbo = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "out_vbo_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "*out_vbo_manager_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != config_->base_align, ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_rslt_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_create", "config_->base_align")
    IF_ARG_FALSE_GOTO_CLEANUP(IS_POWER_OF_TWO(config_->base_align), ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_rslt_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_create", "config_->base_align")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != config_->max_node_count, ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_rslt_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_create", "config_->max_node_count")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != config_->vbo_size, ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_rslt_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_create", "config_->vbo_size")

    ret_memory = memory_system_allocate(sizeof(vbo_manager_t), MEMORY_TAG_RENDERER, (void**)&tmp_vbo_manager);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory) {
        ret = buffer_manager_rslt_convert_choco_memory(ret_memory);
        ERROR_MESSAGE("vbo_manager_create(%s) - vbo manager create failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret_allocator = range_free_list_create(config_->vbo_size, config_->max_node_count, config_->base_align, &tmp_allocator);
    if(RANGE_FREE_LIST_SUCCESS != ret_allocator) {
        ret = buffer_manager_rslt_convert_range_free_list(ret_allocator);
        ERROR_MESSAGE("vbo_manager_create(%s) - vbo manager create failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer = renderer_backend_vertex_buffer_create(backend_context_, &tmp_vbo);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_create(%s) - vbo manager create failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_vbo_manager->config = *config_;
    tmp_vbo_manager->range_free_list = tmp_allocator;
    tmp_vbo_manager->vbo = tmp_vbo;

    *out_vbo_manager_ = tmp_vbo_manager;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_SUCCESS != ret) {
        renderer_backend_vertex_buffer_destroy(backend_context_, &tmp_vbo);
        range_free_list_destroy(&tmp_allocator);
        if(NULL != tmp_vbo_manager) {
            memory_system_free(tmp_vbo_manager, sizeof(vbo_manager_t), MEMORY_TAG_RENDERER);
            tmp_vbo_manager = NULL;
        }
    }
    return ret;
}

void vbo_manager_destroy(renderer_backend_context_t* backend_context_, vbo_manager_t** vbo_manager_) {
    if(NULL == backend_context_) {
        ERROR_MESSAGE("vbo_manager_destroy - VBO cannot be released because backend_context is NULL.");
        return;
    }
    if(NULL == vbo_manager_) {
        return;
    }
    if(NULL == *vbo_manager_) {
        return;
    }
    renderer_backend_vertex_buffer_destroy(backend_context_, &(*vbo_manager_)->vbo);
    range_free_list_destroy(&(*vbo_manager_)->range_free_list);

    memory_system_free(*vbo_manager_, sizeof(vbo_manager_t), MEMORY_TAG_RENDERER);
    *vbo_manager_ = NULL;
}
