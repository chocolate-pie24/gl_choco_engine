// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file resource_registry_err_utils.c
 * @author chocolate-pie24
 *
 * @brief resource_registry_err_utilsは、resource_registries内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @date 2026-06-20
 *
 */
#include "engine/systems/renderer/resources/registries/core/resource_registry_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"

const char* resource_registry_result_to_str(resource_registry_result_t result_) {
    switch(result_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return STRINGIFY(RESOURCE_REGISTRY_SUCCESS);
    case RESOURCE_REGISTRY_NO_MEMORY:
        return STRINGIFY(RESOURCE_REGISTRY_NO_MEMORY);
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return STRINGIFY(RESOURCE_REGISTRY_RUNTIME_ERROR);
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return STRINGIFY(RESOURCE_REGISTRY_INVALID_ARGUMENT);
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return STRINGIFY(RESOURCE_REGISTRY_DATA_CORRUPTED);
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return STRINGIFY(RESOURCE_REGISTRY_BAD_OPERATION);
    case RESOURCE_REGISTRY_OVERFLOW:
        return STRINGIFY(RESOURCE_REGISTRY_OVERFLOW);
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return STRINGIFY(RESOURCE_REGISTRY_LIMIT_EXCEEDED);
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return STRINGIFY(RESOURCE_REGISTRY_UNDEFINED_ERROR);
    default:
        return STRINGIFY(RESOURCE_REGISTRY_UNDEFINED_ERROR);
    }
}

resource_registry_result_t resource_registry_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return RESOURCE_REGISTRY_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return RESOURCE_REGISTRY_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return RESOURCE_REGISTRY_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return RESOURCE_REGISTRY_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return RESOURCE_REGISTRY_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return RESOURCE_REGISTRY_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return RESOURCE_REGISTRY_LIMIT_EXCEEDED;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    default:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    }
}

resource_registry_result_t resource_registry_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_REGISTRY_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_REGISTRY_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_REGISTRY_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_REGISTRY_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_REGISTRY_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_REGISTRY_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_REGISTRY_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_REGISTRY_LIMIT_EXCEEDED;
    default:
        return RESOURCE_REGISTRY_UNDEFINED_ERROR;
    }
}
