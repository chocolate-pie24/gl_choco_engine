#include "engine/systems/renderer/renderer_resources/buffer_managers/vbo_manager.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // for fprintf

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/systems/renderer/renderer_core/allocators/range_free_list.h"

#include "engine/systems/renderer/renderer_core/renderer_geometry_types.h"

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

static bool vbo_manager_is_valid(const vbo_manager_t* vbo_manager_);

buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_free_list_result_t ret_allocator = RANGE_FREE_LIST_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    memory_system_result_t ret_memory = MEMORY_SYSTEM_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;
    range_free_list_t* tmp_allocator = NULL;
    renderer_backend_vbo_t* tmp_vbo = NULL;

    bool vbo_created = false;
    bool vbo_bound = false;

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
    vbo_created = true;

    ret_renderer = renderer_backend_vertex_buffer_bind(backend_context_, tmp_vbo);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to bind vertex buffer.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer = renderer_backend_vertex_buffer_vertex_load(backend_context_, config_->vbo_size, 0, config_->buffer_usage);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create vertex buffer.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to unbind vertex buffer.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    tmp_vbo_manager->config = *config_;
    tmp_vbo_manager->range_free_list = tmp_allocator;
    tmp_vbo_manager->vbo = tmp_vbo;

    *out_vbo_manager_ = tmp_vbo_manager;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_SUCCESS != ret) {
        if(vbo_created) {
            if(vbo_bound) {
                renderer_backend_vertex_buffer_unbind(backend_context_);
            }
            renderer_backend_vertex_buffer_destroy(backend_context_, &tmp_vbo);
        }

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

buffer_manager_result_t vbo_manager_vbo_write(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_, size_t size_, const void* write_data_, vertex_allocation_t* out_allocation_handle_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_free_list_result_t ret_allocator = RANGE_FREE_LIST_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;

    range_allocation_t tmp_allocation = { 0 };

    bool allocate_success = false;
    bool load_success = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_write", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_write", "backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != size_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_write", "size_")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_write", "write_data_")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_handle_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_write", "out_allocation_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(vbo_manager_is_valid(vbo_manager_), ret, BUFFER_MANAGER_DATA_CORRUPTED, buffer_manager_rslt_to_str(BUFFER_MANAGER_DATA_CORRUPTED), "vbo_manager_vbo_write", "vbo_manager_")

    ret_allocator = range_free_list_allocate(vbo_manager_->range_free_list, size_, vbo_manager_->config.base_align, &tmp_allocation);
    if(RANGE_FREE_LIST_SUCCESS != ret_allocator) {
        ret = buffer_manager_rslt_convert_range_free_list(ret_allocator);
        ERROR_MESSAGE("vbo_manager_vbo_write(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    allocate_success = true;

    ret_renderer = renderer_backend_vertex_buffer_bind(backend_context_, vbo_manager_->vbo);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_vbo_write(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer = renderer_backend_vertex_buffer_vertex_subload(backend_context_, tmp_allocation.offset, size_, write_data_);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_vbo_write(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    load_success = true;

    ret_renderer = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_vbo_write(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    out_allocation_handle_->allocation_size = tmp_allocation.allocated_size;
    out_allocation_handle_->vertex_offset = tmp_allocation.offset;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    // NOTE: allocateで失敗した場合、range_free_list側で解放するサイズが不明(アライメントされるためsize_と異なる場合がある)なためローバック不可
    // NOTE: Range Free Listを、allocateから2-phase allocationに仕様変更し、ロールバック処理を変更する
    if(allocate_success) {
        if(!load_success || vbo_bound) {    // vbo_bindに失敗 or subloadに失敗 or vbo_unbindに失敗
            ret_allocator = range_free_list_free(vbo_manager_->range_free_list, tmp_allocation);
            if(RANGE_FREE_LIST_SUCCESS != ret_allocator) {
                ERROR_MESSAGE("vbo_manager_vbo_write(%s) - range_free_list_free failed.", buffer_manager_rslt_to_str(buffer_manager_rslt_convert_range_free_list(ret_allocator)));
            }
        }
        if(vbo_bound) {
            ret_renderer = renderer_backend_vertex_buffer_unbind(backend_context_);
            if(RENDERER_SUCCESS != ret_renderer) {
                ERROR_MESSAGE("vbo_manager_vbo_write(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(buffer_manager_rslt_convert_renderer(ret_renderer)));
            }
        }
    }

    return ret;
}

buffer_manager_result_t vbo_manager_vbo_free(vbo_manager_t* vbo_manager_, const vertex_allocation_t* allocation_handle_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_free_list_result_t ret_allocator = RANGE_FREE_LIST_INVALID_ARGUMENT;

    range_allocation_t tmp_range = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_free", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_handle_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_free", "allocation_handle_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != allocation_handle_->allocation_size, ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_rslt_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_vbo_free", "range_.allocated_size")
    IF_ARG_FALSE_GOTO_CLEANUP(vbo_manager_is_valid(vbo_manager_), ret, BUFFER_MANAGER_DATA_CORRUPTED, buffer_manager_rslt_to_str(BUFFER_MANAGER_DATA_CORRUPTED), "vbo_manager_vbo_free", "vbo_manager_")

    tmp_range.allocated_size = allocation_handle_->allocation_size;
    tmp_range.offset = allocation_handle_->vertex_offset;

    ret_allocator = range_free_list_free(vbo_manager_->range_free_list, tmp_range);
    if(RANGE_FREE_LIST_SUCCESS != ret_allocator) {
        ret = buffer_manager_rslt_convert_range_free_list(ret_allocator);
        ERROR_MESSAGE("vbo_manager_vbo_free(%s) - vbo_manager_vbo_free failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t vbo_manager_vbo_bind(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_bind", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_bind", "backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(vbo_manager_is_valid(vbo_manager_), ret, BUFFER_MANAGER_DATA_CORRUPTED, buffer_manager_rslt_to_str(BUFFER_MANAGER_DATA_CORRUPTED), "vbo_manager_vbo_bind", "vbo_manager_")

    ret_renderer = renderer_backend_vertex_buffer_bind(backend_context_, vbo_manager_->vbo);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_vbo_bind(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t vbo_manager_vbo_unbind(const renderer_backend_context_t* backend_context_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_rslt_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_vbo_unbind", "backend_context_")

    ret_renderer = renderer_backend_vertex_buffer_unbind(backend_context_);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = buffer_manager_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("vbo_manager_vbo_unbind(%s) - vbo_manager_vbo_write failed.", buffer_manager_rslt_to_str(ret));
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

void vbo_manager_status_print(const vbo_manager_t* vbo_manager_) {
    range_free_list_status_t status = { 0 };

    const bool valid = vbo_manager_is_valid(vbo_manager_);

    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる
    fprintf(stdout, "\033[1;35m[VBO MANAGER DUMP MESSAGE]\n");
    if(!valid) {
        fprintf(stdout, "  vbo_manager_is_valid = false\n");
    } else {
        range_free_list_status_get(vbo_manager_->range_free_list, &status);
        fprintf(stdout, "  vbo_manager_is_valid = true\n");
        fprintf(stdout, "  vbo_size = %zu\n", vbo_manager_->config.vbo_size);
        fprintf(stdout, "  max_node_count = %zu\n", vbo_manager_->config.max_node_count);
        fprintf(stdout, "  buffer_usage = %s\n", BUFFER_USAGE_STATIC == vbo_manager_->config.buffer_usage ? "STATIC" : "DYNAMIC");
        fprintf(stdout, "  range_free_list status:\n");
        fprintf(stdout, "    base_align = %zu\n", vbo_manager_->config.base_align);
        fprintf(stdout, "    memory_pool_size = %zu\n", status.memory_pool_size);
        fprintf(stdout, "    base_align = %zu\n", status.base_align);
        fprintf(stdout, "    max_node_count = %zu\n", status.max_node_count);
        fprintf(stdout, "    unused_node_count = %zu\n", status.unused_node_count);
        fprintf(stdout, "    free_block_count = %zu\n", status.free_block_count);
        fprintf(stdout, "    total_free_size = %zu\n", status.total_free_size);
        fprintf(stdout, "    max_free_block_size = %zu\n", status.max_free_block_size);
    }

    fprintf(stdout, "\033[0m");
    funlockfile(stdout);
}

void vbo_manager_debug_print(const vbo_manager_t* vbo_manager_) {
    range_free_list_status_t status = { 0 };

    const bool valid = vbo_manager_is_valid(vbo_manager_);

    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる
    fprintf(stdout, "\033[1;35m[VBO MANAGER DUMP MESSAGE]\n");
    if(!valid) {
        fprintf(stdout, "  vbo_manager_is_valid = false\n");
    } else {
        range_free_list_status_get(vbo_manager_->range_free_list, &status);
        fprintf(stdout, "  vbo_manager_is_valid = true\n");
        fprintf(stdout, "  vbo_size = %zu\n", vbo_manager_->config.vbo_size);
        fprintf(stdout, "  max_node_count = %zu\n", vbo_manager_->config.max_node_count);
    }
    fprintf(stdout, "\033[0m");
    funlockfile(stdout);

    if(valid) {
        range_free_list_debug_print(vbo_manager_->range_free_list);
    }
}

static bool vbo_manager_is_valid(const vbo_manager_t* vbo_manager_) {
    if(NULL == vbo_manager_) {
        return false;
    }
    if(NULL == vbo_manager_->range_free_list) {
        return false;
    }
    if(NULL == vbo_manager_->vbo) {
        return false;
    }
    if(0 == vbo_manager_->config.vbo_size) {
        return false;
    }
    if(0 == vbo_manager_->config.max_node_count) {
        return false;
    }
    if(0 == vbo_manager_->config.base_align || !IS_POWER_OF_TWO(vbo_manager_->config.base_align)) {
        return false;
    }
    return true;
}
