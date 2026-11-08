// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file vbo_manager.c
 * @brief GPU側VBOとその内部rangeを管理するVBO Managerの実装
 *
 * @details
 * vbo_manager.hで公開するVBO Manager APIを実装する。
 *
 * 一つのVBO Managerにつき、一つのRenderer Backend VBOと
 * 一つのRange Allocatorを所有する。
 * GPU buffer objectに対する操作はRenderer Backendへ委譲し、
 * VBO内の論理的なoffset、実確保サイズ、およびallocation identityは
 * Range Allocatorを使用して管理する。
 *
 * @par Resource生成
 * vbo_manager_create()は、次の順序で内部リソースを生成する。
 *
 * 1. VBO Manager本体のメモリを確保する
 * 2. Range Allocatorを生成する
 * 3. Renderer Backend VBOを生成する
 * 4. VBOをbindする
 * 5. GPU側VBOのbuffer領域を確保する
 * 6. VBOをunbindする
 * 7. 完成したVBO Managerを呼び出し側へ公開する
 *
 * 途中で失敗した場合は、生成済みリソースを解放し、
 * 未完成のVBO Managerを呼び出し側へ公開しない。
 *
 * @par Write処理
 * vbo_manager_write()は、次の順序でVBOへデータを転送する。
 *
 * 1. Range AllocatorからVBO内rangeを確保する
 * 2. 対象VBOをbindする
 * 3. 確保したrangeのoffsetへデータを転送する
 * 4. VBOをunbindする
 * 5. allocation handleを呼び出し側へ公開する
 *
 * range確保後の処理に失敗した場合は、確保済みrangeをfreeし、
 * 必要に応じてVBOをunbindする。
 *
 * rollback中に、正常な内部状態では成功するはずのrange解放または
 * unbindに失敗した場合は、内部状態を保証できないため
 * BUFFER_MANAGER_DATA_CORRUPTEDを返す。
 *
 * GPU側へ転送済みのデータは復元または消去しない。
 * write失敗時にはallocation handleが公開されず、対応rangeが
 * 再利用対象となるため、論理的なallocationのrollbackだけを行う。
 *
 * @par Free処理
 * vbo_manager_free()は、allocation handleが表すrangeを
 * Range Allocatorへ返却する。
 *
 * GPU側VBOに残っているデータの消去またはゼロクリアは行わない。
 * freeされたrangeは、後続のwriteによって再利用される。
 *
 * @par Validation
 * vbo_manager_config_is_valid()は、VBO Manager生成設定の
 * 基本的な有効性を検証する。
 *
 * 公開APIの入口では、NULL、size、および基本的な事前条件を検証する。
 *
 * privateなvbo_manager_is_valid()は、VBO Manager本体、
 * 所有する内部リソースへのpointer、および保持しているconfigの
 * 基本的な有効性を検証する。
 *
 * 現時点では、Range Allocator内部のrange listおよびnode pool、
 * Renderer Backend VBOの内部状態、ならびに実際のGPU側VBOの状態に
 * 対するdeep validationは行わない。
 *
 * @date 2026-07-31
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // for fprintf

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/low_level_allocators/range_allocator/range_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vbo.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"
#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_err_utils.h"

/**
 * @brief VBO Managerの内部状態管理構造体
 *
 * @details
 * VBO Managerの生成設定、VBO内rangeを管理するRange Allocator、
 * およびGPU側VBOを操作するRenderer Backend VBOを保持する。
 *
 * 本構造体はvbo_manager_create()によって動的に生成され、
 * vbo_manager_destroy()によって内部リソースとともに破棄される。
 *
 * @par 内部不変条件
 * 正常に生成されたVBO Managerでは、次の条件が成立する。
 *
 * - vbo_manager_config_is_valid(&config)がtrueを返す
 * - range_allocatorはNULLではない
 * - vboはNULLではない
 * - range_allocatorはconfigに指定されたVBO size、allocation数上限、
 *   およびbase alignmentを使用して生成されている
 * - GPU側VBOにはconfig.vbo_sizeのbuffer領域が確保されている
 *
 * @par Ownership
 * 本構造体はrange_allocatorおよびvboを所有する。
 * configは呼び出し側から値としてコピーしたものであり、
 * 呼び出し側のvbo_manager_config_tへのpointerは保持しない。
 *
 * Renderer Backend Contextは所有および保持せず、必要な公開APIの
 * 呼び出し時に外部から受け取る。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
struct vbo_manager {
    renderer_backend_context_t* backend_context;
    vbo_manager_config_t config;            /**< VBO Manager生成時にコピーした設定 */
    range_allocator_t* range_allocator;     /**< VBO内の論理rangeを管理するRange Allocator */
    renderer_backend_vbo_t* vbo;            /**< 所有するRenderer Backend VBO */
};

buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_allocator_result_t ret_range_allocator = RANGE_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    vbo_manager_t* tmp_vbo_manager = NULL;
    range_allocator_t* tmp_allocator = NULL;
    renderer_backend_vbo_t* tmp_vbo = NULL;

    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_create", "out_vbo_manager_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vbo_manager_, ret, BUFFER_MANAGER_BAD_OPERATION, buffer_manager_result_to_str(BUFFER_MANAGER_BAD_OPERATION), "vbo_manager_create", "*out_vbo_manager_")
    if(!vbo_manager_config_is_valid(config_)) {
        ret = BUFFER_MANAGER_INVALID_ARGUMENT;
        ERROR_MESSAGE("vbo_manager_create(%s) - Provided config_ is not valid.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(vbo_manager_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_vbo_manager);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = buffer_manager_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("vbo_manager_create(%s) - general_allocator_allocate failed.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }

    ret_range_allocator = range_allocator_create(config_->vbo_size, config_->max_allocation_count, config_->base_align, &tmp_allocator);
    if(RANGE_ALLOCATOR_SUCCESS != ret_range_allocator) {
        ret = buffer_manager_result_convert_range_allocator(ret_range_allocator);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create VBO Manager. reason=range_allocator_create_failed, vbo_size=%zu, max_allocation_count=%zu, base_align=%zu, range_allocator_result=%d", buffer_manager_result_to_str(ret), config_->vbo_size, config_->max_allocation_count, config_->base_align, (int)ret_range_allocator);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_create(backend_context_, &tmp_vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_create_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_bind(backend_context_, tmp_vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_bind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vbo_vertex_load(backend_context_, config_->vbo_size, 0, config_->buffer_usage);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_vertex_load_failed, vbo_size=%zu, buffer_usage=%d, renderer_result=%d", buffer_manager_result_to_str(ret), config_->vbo_size, (int)config_->buffer_usage, (int)ret_renderer_backend);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_create(%s) - Failed to create VBO Manager. reason=renderer_backend_vbo_unbind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = false;

    tmp_vbo_manager->config = *config_;
    tmp_vbo_manager->range_allocator = tmp_allocator;
    tmp_vbo_manager->vbo = tmp_vbo;
    tmp_vbo_manager->backend_context = backend_context_;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(tmp_vbo_manager)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_create(%s) - Result validation failed for 'tmp_vbo_manager'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_vbo_manager_ = tmp_vbo_manager;
    tmp_vbo_manager = NULL;
    tmp_allocator = NULL;
    tmp_vbo = NULL;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_DATA_CORRUPTED == ret || BUFFER_MANAGER_SUCCESS == ret) {
        return ret;
    }
    if(vbo_bound) {
        ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("vbo_manager_create(%s) - Failed to rollback VBO Manager creation. reason=renderer_backend_vbo_unbind_failed, original_result=%s, renderer_result=%d", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED), buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }
    if(NULL != tmp_vbo) {
        renderer_backend_vbo_destroy(backend_context_, &tmp_vbo);
    }
    if(NULL != tmp_allocator) {
        range_allocator_destroy(&tmp_allocator);
    }
    if(NULL != tmp_vbo_manager) {
        general_allocator_free((void**)&tmp_vbo_manager, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void vbo_manager_destroy(vbo_manager_t** vbo_manager_) {
    if(NULL == vbo_manager_) {
        return;
    }
    if(NULL == *vbo_manager_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(*vbo_manager_)) {
        ERROR_MESSAGE("vbo_manager_destroy(%s) - Precondition validation failed for '*vbo_manager_'.", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED));
        return;
    }
#endif
    renderer_backend_vbo_destroy((*vbo_manager_)->backend_context, &(*vbo_manager_)->vbo);
    range_allocator_destroy(&(*vbo_manager_)->range_allocator);

    general_allocator_free((void**)vbo_manager_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

buffer_manager_result_t vbo_manager_write(vbo_manager_t* vbo_manager_, size_t size_, const void* write_data_, range_allocation_t* out_allocation_handle_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_allocator_result_t ret_range_allocator = RANGE_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    range_allocation_t tmp_allocation = { 0 };

    bool allocate_success = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_write", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(write_data_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_write", "write_data_")
    IF_ARG_NULL_GOTO_CLEANUP(out_allocation_handle_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_write", "out_allocation_handle_")
    if(0 == size_) {
        ret = BUFFER_MANAGER_INVALID_ARGUMENT;
        ERROR_MESSAGE("vbo_manager_write(%s) - Provided size_ is not valid.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(vbo_manager_)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_write(%s) - Precondition validation failed for 'vbo_manager_'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(0 != (size_ % vbo_manager_->config.base_align)) {
        ret = BUFFER_MANAGER_BAD_OPERATION;
        ERROR_MESSAGE("vbo_manager_write(%s) - Failed to write vertex data. reason=write_size_misaligned, write_size=%zu, base_align=%zu", buffer_manager_result_to_str(ret), size_, vbo_manager_->config.base_align);
        goto cleanup;
    }

    ret_range_allocator = range_allocator_allocate(vbo_manager_->range_allocator, size_, vbo_manager_->config.base_align, &tmp_allocation);
    if(RANGE_ALLOCATOR_SUCCESS != ret_range_allocator) {
        ret = buffer_manager_result_convert_range_allocator(ret_range_allocator);
        ERROR_MESSAGE("vbo_manager_write(%s) - Failed to write vertex data. reason=range_allocator_allocate_failed, write_size=%zu, base_align=%zu, range_allocator_result=%d", buffer_manager_result_to_str(ret), size_, vbo_manager_->config.base_align, (int)ret_range_allocator);
        goto cleanup;
    }
    allocate_success = true;

    ret_renderer_backend = renderer_backend_vbo_bind(vbo_manager_->backend_context, vbo_manager_->vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_write(%s) - Failed to write vertex data. reason=renderer_backend_vbo_bind_failed, allocation_offset=%zu, allocation_size=%zu, renderer_result=%d", buffer_manager_result_to_str(ret), tmp_allocation.offset, tmp_allocation.allocated_size, (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = true;

    ret_renderer_backend = renderer_backend_vbo_vertex_subload(vbo_manager_->backend_context, tmp_allocation.offset, size_, write_data_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_write(%s) - Failed to write vertex data. reason=renderer_backend_vbo_vertex_subload_failed, write_offset=%zu, write_size=%zu, renderer_result=%d", buffer_manager_result_to_str(ret), tmp_allocation.offset, size_, (int)ret_renderer_backend);
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_write(%s) - Failed to write vertex data. reason=renderer_backend_vbo_unbind_failed, allocation_offset=%zu, allocation_size=%zu, renderer_result=%d", buffer_manager_result_to_str(ret), tmp_allocation.offset, tmp_allocation.allocated_size, (int)ret_renderer_backend);
        goto cleanup;
    }
    vbo_bound = false;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(vbo_manager_)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_write(%s) - Postcondition validation failed for 'vbo_manager_'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_allocation_handle_ = tmp_allocation;

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    if(BUFFER_MANAGER_DATA_CORRUPTED == ret || BUFFER_MANAGER_SUCCESS == ret) {
        return ret;
    }
    if(allocate_success) {
        // range_allocatorの内部状態破損がなければ成功するはず。失敗はDATA_CORRUPTEDとする
        ret_range_allocator = range_allocator_free(vbo_manager_->range_allocator, &tmp_allocation);
        if(RANGE_ALLOCATOR_SUCCESS != ret_range_allocator) {
            ERROR_MESSAGE("vbo_manager_write(%s) - Failed to rollback vertex data write. reason=range_allocator_free_failed, original_result=%s, allocation_offset=%zu, allocation_size=%zu, range_allocator_result=%d", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED), buffer_manager_result_to_str(ret), tmp_allocation.offset, tmp_allocation.allocated_size, (int)ret_range_allocator);
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }
    if(vbo_bound) {
        // backend_context_の内部状態破損がなければ成功するはず。失敗はDATA_CORRUPTEDとする
        ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ERROR_MESSAGE("vbo_manager_write(%s) - Failed to rollback vertex data write. reason=renderer_backend_vbo_unbind_failed, previous_result=%s, allocation_offset=%zu, allocation_size=%zu, renderer_result=%d", buffer_manager_result_to_str(BUFFER_MANAGER_DATA_CORRUPTED), buffer_manager_result_to_str(ret), tmp_allocation.offset, tmp_allocation.allocated_size, (int)ret_renderer_backend);
            ret = BUFFER_MANAGER_DATA_CORRUPTED;
            return ret;
        }
    }

    return ret;
}

buffer_manager_result_t vbo_manager_free(vbo_manager_t* vbo_manager_, const range_allocation_t* allocation_handle_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    range_allocator_result_t ret_range_allocator = RANGE_ALLOCATOR_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_free", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(allocation_handle_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_free", "allocation_handle_")
    if(!range_allocation_is_valid(allocation_handle_)) {    // NOTE: O(1)なのと、API誤用がありうるため全ビルドで実行
        ret = BUFFER_MANAGER_INVALID_ARGUMENT;
        ERROR_MESSAGE("vbo_manager_free(%s) - Provided allocation_handle_ is not valid.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(vbo_manager_)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_free(%s) - Precondition validation failed for 'vbo_manager_'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    // NOTE:
    // - range_allocatorの内部データ不整合: DATA_CORRUPTED
    // - vbo_manager_とallocation_handle_の不整合: BAD_OPERATION
    ret_range_allocator = range_allocator_free(vbo_manager_->range_allocator, allocation_handle_);
    if(RANGE_ALLOCATOR_SUCCESS != ret_range_allocator) {
        ret = buffer_manager_result_convert_range_allocator(ret_range_allocator);
        ERROR_MESSAGE("vbo_manager_free(%s) - Failed to free vertex allocation. reason=range_allocator_free_failed, allocation_offset=%zu, allocation_size=%zu, range_allocator_result=%d", buffer_manager_result_to_str(ret), allocation_handle_->offset, allocation_handle_->allocated_size, (int)ret_range_allocator);
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!vbo_manager_is_valid(vbo_manager_)) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_free(%s) - Postcondition validation failed for 'vbo_manager_'.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t vbo_manager_bind(vbo_manager_t* vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_bind", "vbo_manager_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(NULL == vbo_manager_->backend_context || NULL == vbo_manager_->vbo) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_bind(%s) - Provided vbo_manager_ is corrupted.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_vbo_bind(vbo_manager_->backend_context, vbo_manager_->vbo);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_bind(%s) - Failed to bind vertex buffer. reason=renderer_backend_vbo_bind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

buffer_manager_result_t vbo_manager_unbind(const vbo_manager_t* vbo_manager_) {
    buffer_manager_result_t ret = BUFFER_MANAGER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, BUFFER_MANAGER_INVALID_ARGUMENT, buffer_manager_result_to_str(BUFFER_MANAGER_INVALID_ARGUMENT), "vbo_manager_unbind", "vbo_manager_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(NULL == vbo_manager_->backend_context) {
        ret = BUFFER_MANAGER_DATA_CORRUPTED;
        ERROR_MESSAGE("vbo_manager_unbind(%s) - Provided vbo_manager_ is corrupted.", buffer_manager_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_vbo_unbind(vbo_manager_->backend_context);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = buffer_manager_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("vbo_manager_unbind(%s) - Failed to unbind vertex buffer. reason=renderer_backend_vbo_unbind_failed, renderer_result=%d", buffer_manager_result_to_str(ret), (int)ret_renderer_backend);
        goto cleanup;
    }

    ret = BUFFER_MANAGER_SUCCESS;

cleanup:
    return ret;
}

const renderer_backend_vbo_t* vbo_manager_vbo_get(const vbo_manager_t* vbo_manager_) {
    if(NULL == vbo_manager_) {
        return NULL;
    }
    return vbo_manager_->vbo;
}

bool vbo_manager_config_is_valid(const vbo_manager_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_allocation_count) {
        return false;
    }
    if(0 == config_->base_align || !IS_POWER_OF_TWO(config_->base_align)) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool vbo_manager_is_valid(const vbo_manager_t* vbo_manager_) {
    range_allocator_status_t status = { 0 };

    if(NULL == vbo_manager_) {
        return false;
    }

    if(NULL == vbo_manager_->backend_context) {
        return false;
    }

    if(NULL == vbo_manager_->range_allocator || NULL == vbo_manager_->vbo) {
        return false;
    }

    if(!vbo_manager_config_is_valid(&vbo_manager_->config)) {
        return false;
    }

    // Owned ResourceのAllocation Validity
    if(!general_allocator_ptr_is_allocated(vbo_manager_->range_allocator)) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(vbo_manager_->vbo)) {
        return false;
    }

    // Owned ResourceのCanonical Validity
    if(!range_allocator_is_valid(vbo_manager_->range_allocator)) {
        return false;
    }

    if(!renderer_backend_vbo_is_valid(
        vbo_manager_->backend_context,
        vbo_manager_->vbo)) {
        return false;
    }

    // ConfigとRange Allocatorの整合性
    range_allocator_status_get(vbo_manager_->range_allocator, &status);

    if(vbo_manager_->config.vbo_size != status.memory_pool_size) {
        return false;
    }

    if(vbo_manager_->config.max_allocation_count != status.max_allocation_count) {
        return false;
    }

    if(vbo_manager_->config.base_align != status.base_align) {
        return false;
    }

    return true;
}

void vbo_manager_status_print(const vbo_manager_t* vbo_manager_) {
    range_allocator_status_t status = { 0 };

    const bool valid = vbo_manager_is_valid(vbo_manager_);

    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる
    fprintf(stdout, "\033[1;35m[VBO MANAGER STATUS]\n");
    if(!valid) {
        fprintf(stdout, "  vbo_manager_is_valid = false\n");
    } else {
        fprintf(stdout, "  vbo_manager_is_valid = true\n");
        fprintf(stdout, "  vbo_size = %zu\n", vbo_manager_->config.vbo_size);
        fprintf(stdout, "  max_allocation_count = %zu\n", vbo_manager_->config.max_allocation_count);
        fprintf(stdout, "  buffer_usage = %s\n", BUFFER_USAGE_STATIC == vbo_manager_->config.buffer_usage ? "STATIC" : "DYNAMIC");

        range_allocator_status_get(vbo_manager_->range_allocator, &status);
    }
    fprintf(stdout, "\033[0m");
    funlockfile(stdout);
    if(valid) {
        range_allocator_status_print(&status);
    }
}

void vbo_manager_debug_print(const vbo_manager_t* vbo_manager_) {
    const bool valid = vbo_manager_is_valid(vbo_manager_);

    flockfile(stdout); // 同一ストリームの同時書き込みをまとめる
    fprintf(stdout, "\033[1;35m[VBO MANAGER DEBUG DUMP]\n");
    if(!valid) {
        fprintf(stdout, "  vbo_manager_is_valid = false\n");
    } else {
        fprintf(stdout, "  vbo_manager_is_valid = true\n");
        fprintf(stdout, "  vbo_size = %zu\n", vbo_manager_->config.vbo_size);
        fprintf(stdout, "  max_allocation_count = %zu\n", vbo_manager_->config.max_allocation_count);
        fprintf(stdout, "  buffer_usage = %s\n", BUFFER_USAGE_STATIC == vbo_manager_->config.buffer_usage ? "STATIC" : "DYNAMIC");
    }
    fprintf(stdout, "\033[0m");
    funlockfile(stdout);
    if(valid) {
        range_allocator_debug_print(vbo_manager_->range_allocator);
    }
}
