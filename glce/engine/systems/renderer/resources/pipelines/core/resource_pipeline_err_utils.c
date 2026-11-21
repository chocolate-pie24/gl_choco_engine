// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

const char* resource_pipeline_result_to_str(resource_pipeline_result_t result_) {
    switch(result_) {
    case RESOURCE_PIPELINE_SUCCESS:
        return STRINGIFY(RESOURCE_PIPELINE_SUCCESS);
    case RESOURCE_PIPELINE_NO_MEMORY:
        return STRINGIFY(RESOURCE_PIPELINE_NO_MEMORY);
    case RESOURCE_PIPELINE_RUNTIME_ERROR:
        return STRINGIFY(RESOURCE_PIPELINE_RUNTIME_ERROR);
    case RESOURCE_PIPELINE_INVALID_ARGUMENT:
        return STRINGIFY(RESOURCE_PIPELINE_INVALID_ARGUMENT);
    case RESOURCE_PIPELINE_DATA_CORRUPTED:
        return STRINGIFY(RESOURCE_PIPELINE_DATA_CORRUPTED);
    case RESOURCE_PIPELINE_BAD_OPERATION:
        return STRINGIFY(RESOURCE_PIPELINE_BAD_OPERATION);
    case RESOURCE_PIPELINE_OVERFLOW:
        return STRINGIFY(RESOURCE_PIPELINE_OVERFLOW);
    case RESOURCE_PIPELINE_LIMIT_EXCEEDED:
        return STRINGIFY(RESOURCE_PIPELINE_LIMIT_EXCEEDED);
    case RESOURCE_PIPELINE_FILE_OPEN_ERROR:
        return STRINGIFY(RESOURCE_PIPELINE_FILE_OPEN_ERROR);
    case RESOURCE_PIPELINE_FILE_READ_ERROR:
        return STRINGIFY(RESOURCE_PIPELINE_FILE_READ_ERROR);
    case RESOURCE_PIPELINE_UNSUPPORTED_FILE:
        return STRINGIFY(RESOURCE_PIPELINE_UNSUPPORTED_FILE);
    case RESOURCE_PIPELINE_UNDEFINED_ERROR:
        return STRINGIFY(RESOURCE_PIPELINE_UNDEFINED_ERROR);
    default:
        return STRINGIFY(RESOURCE_PIPELINE_UNDEFINED_ERROR);
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_geometry(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case GEOMETRY_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case GEOMETRY_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case GEOMETRY_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case GEOMETRY_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case GEOMETRY_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case GEOMETRY_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case GEOMETRY_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case GEOMETRY_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

// TODO: シェーダーリソースはパイプラインでは扱わない(シェーダーは各プログラムにつき1個のため、registryを作るまでもない、であればpipelineも作るまでもない)
resource_pipeline_result_t resource_pipeline_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_resource_registry(resource_registry_result_t result_) {
    switch(result_) {
    case RESOURCE_REGISTRY_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case RESOURCE_REGISTRY_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case RESOURCE_REGISTRY_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case RESOURCE_REGISTRY_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case RESOURCE_REGISTRY_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case RESOURCE_REGISTRY_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case RESOURCE_REGISTRY_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case RESOURCE_REGISTRY_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case RESOURCE_REGISTRY_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_shader(shader_result_t result_) {
    switch(result_) {
    case SHADER_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case SHADER_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case SHADER_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case SHADER_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case SHADER_COMPILE_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case SHADER_LINK_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR; // Resource Pipelineではシェーダーのコンパイル, リンクを行わないためUNDEFINED_ERRORに変換
    case SHADER_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case SHADER_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case SHADER_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case SHADER_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case SHADER_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_choco_string(choco_string_result_t result_) {
    switch(result_) {
    case CHOCO_STRING_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case CHOCO_STRING_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case CHOCO_STRING_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case CHOCO_STRING_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case CHOCO_STRING_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case CHOCO_STRING_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case CHOCO_STRING_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case CHOCO_STRING_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case CHOCO_STRING_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_fs_stream(fs_stream_result_t result_) {
    switch(result_) {
    case FS_STREAM_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case FS_STREAM_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case FS_STREAM_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case FS_STREAM_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case FS_STREAM_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case FS_STREAM_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case FS_STREAM_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case FS_STREAM_FILE_OPEN_ERROR:
        return RESOURCE_PIPELINE_FILE_OPEN_ERROR;
    case FS_STREAM_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case FS_STREAM_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    case FS_STREAM_EOF:
        return RESOURCE_PIPELINE_FILE_READ_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_texture(texture_result_t result_) {
    switch(result_) {
    case TEXTURE_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case TEXTURE_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case TEXTURE_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case TEXTURE_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case TEXTURE_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case TEXTURE_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case TEXTURE_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case TEXTURE_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case TEXTURE_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_loader(loader_result_t result_) {
    switch(result_) {
    case LOADER_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case LOADER_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case LOADER_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case LOADER_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case LOADER_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case LOADER_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case LOADER_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case LOADER_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case LOADER_FILE_OPEN_ERROR:
        return RESOURCE_PIPELINE_FILE_OPEN_ERROR;
    case LOADER_FILE_READ_ERROR:
        return RESOURCE_PIPELINE_FILE_READ_ERROR;
    case LOADER_UNSUPPORTED_FILE:
        return RESOURCE_PIPELINE_UNSUPPORTED_FILE;
    case LOADER_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}

resource_pipeline_result_t resource_pipeline_result_convert_buffer_manager(buffer_manager_result_t result_) {
    switch(result_) {
    case BUFFER_MANAGER_SUCCESS:
        return RESOURCE_PIPELINE_SUCCESS;
    case BUFFER_MANAGER_INVALID_ARGUMENT:
        return RESOURCE_PIPELINE_INVALID_ARGUMENT;
    case BUFFER_MANAGER_RUNTIME_ERROR:
        return RESOURCE_PIPELINE_RUNTIME_ERROR;
    case BUFFER_MANAGER_LIMIT_EXCEEDED:
        return RESOURCE_PIPELINE_LIMIT_EXCEEDED;
    case BUFFER_MANAGER_NO_MEMORY:
        return RESOURCE_PIPELINE_NO_MEMORY;
    case BUFFER_MANAGER_DATA_CORRUPTED:
        return RESOURCE_PIPELINE_DATA_CORRUPTED;
    case BUFFER_MANAGER_BAD_OPERATION:
        return RESOURCE_PIPELINE_BAD_OPERATION;
    case BUFFER_MANAGER_OVERFLOW:
        return RESOURCE_PIPELINE_OVERFLOW;
    case BUFFER_MANAGER_UNDEFINED_ERROR:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    default:
        return RESOURCE_PIPELINE_UNDEFINED_ERROR;
    }
}
