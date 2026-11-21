// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/loaders/core/loader_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"

const char* loader_result_to_str(loader_result_t result_) {
    switch(result_) {
    case LOADER_SUCCESS:
        return STRINGIFY(LOADER_SUCCESS);
    case LOADER_NO_MEMORY:
        return STRINGIFY(LOADER_NO_MEMORY);
    case LOADER_RUNTIME_ERROR:
        return STRINGIFY(LOADER_RUNTIME_ERROR);
    case LOADER_INVALID_ARGUMENT:
        return STRINGIFY(LOADER_INVALID_ARGUMENT);
    case LOADER_DATA_CORRUPTED:
        return STRINGIFY(LOADER_DATA_CORRUPTED);
    case LOADER_BAD_OPERATION:
        return STRINGIFY(LOADER_BAD_OPERATION);
    case LOADER_OVERFLOW:
        return STRINGIFY(LOADER_OVERFLOW);
    case LOADER_LIMIT_EXCEEDED:
        return STRINGIFY(LOADER_LIMIT_EXCEEDED);
    case LOADER_FILE_OPEN_ERROR:
        return STRINGIFY(LOADER_FILE_OPEN_ERROR);
    case LOADER_FILE_READ_ERROR:
        return STRINGIFY(LOADER_FILE_READ_ERROR);
    case LOADER_UNSUPPORTED_FILE:
        return STRINGIFY(LOADER_UNSUPPORTED_FILE);
    case LOADER_UNDEFINED_ERROR:
        return STRINGIFY(LOADER_UNDEFINED_ERROR);
    default:
        return STRINGIFY(LOADER_UNDEFINED_ERROR);
    }
}

loader_result_t loader_result_convert_fs_stream(fs_stream_result_t result_) {
    switch(result_) {
    case FS_STREAM_SUCCESS:
        return LOADER_SUCCESS;
    case FS_STREAM_INVALID_ARGUMENT:
        return LOADER_INVALID_ARGUMENT;
    case FS_STREAM_BAD_OPERATION:
        return LOADER_BAD_OPERATION;
    case FS_STREAM_DATA_CORRUPTED:
        return LOADER_DATA_CORRUPTED;
    case FS_STREAM_NO_MEMORY:
        return LOADER_NO_MEMORY;
    case FS_STREAM_LIMIT_EXCEEDED:
        return LOADER_LIMIT_EXCEEDED;
    case FS_STREAM_OVERFLOW:
        return LOADER_OVERFLOW;
    case FS_STREAM_FILE_OPEN_ERROR:
        return LOADER_FILE_OPEN_ERROR;
    case FS_STREAM_RUNTIME_ERROR:
        return LOADER_RUNTIME_ERROR;
    case FS_STREAM_EOF:
        return LOADER_RUNTIME_ERROR;  // 基本的にEOFをそのまま伝播させることはないのでとりあえずRUNTIME_ERRORに変換する
    case FS_STREAM_UNDEFINED_ERROR:
        return LOADER_UNDEFINED_ERROR;
    default:
        return LOADER_UNDEFINED_ERROR;
    }
}

loader_result_t loader_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return LOADER_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return LOADER_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return LOADER_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return LOADER_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return LOADER_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return LOADER_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return LOADER_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return LOADER_UNDEFINED_ERROR;
    default:
        return LOADER_UNDEFINED_ERROR;
    }
}

loader_result_t loader_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return LOADER_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return LOADER_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return LOADER_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return LOADER_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return LOADER_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return LOADER_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return LOADER_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return LOADER_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return LOADER_LIMIT_EXCEEDED;
    default:
        return LOADER_UNDEFINED_ERROR;
    }
}

loader_result_t loader_result_convert_glce_config_utility(glce_config_utility_result_t result_) {
    switch(result_) {
    case GLCE_CONFIG_UTILITY_SUCCESS:
        return LOADER_SUCCESS;
    case GLCE_CONFIG_UTILITY_INVALID_ARGUMENT:
        return LOADER_INVALID_ARGUMENT;
    case GLCE_CONFIG_UTILITY_BAD_OPERATION:
        return LOADER_BAD_OPERATION;
    case GLCE_CONFIG_UTILITY_UNDEFINED_ERROR:
        return LOADER_UNDEFINED_ERROR;
    case GLCE_CONFIG_UTILITY_UNSUPPORTED_FORMAT:
        return LOADER_UNSUPPORTED_FILE;
    default:
        return LOADER_UNDEFINED_ERROR;
    }
}
