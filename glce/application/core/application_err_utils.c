// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup application
 *
 * @file application_err_utils.c
 * @author chocolate-pie24
 * @brief アプリケーションレイヤー内でのエラー処理仕様を統一するため、実行結果コード変換機能の実装
 *
 * @date 2026-03-25
 *
 */
#include "application/core/application_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/memory/general_allocator/general_allocator.h"
#include "engine/memory/subsystem_allocator/subsystem_allocator.h"

#include "engine/containers/ring_queue.h"

#include "engine/io_utils/fs_path.h"

#include "engine/systems/platform_system/core/platform_system_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"

#include "application/core/application_types.h"

#include "application/scenes/core/scene_types.h"

const char* application_result_to_str(application_result_t result_) {
    switch(result_) {
    case APPLICATION_SUCCESS:
        return STRINGIFY(APPLICATION_SUCCESS);
    case APPLICATION_NO_MEMORY:
        return STRINGIFY(APPLICATION_NO_MEMORY);
    case APPLICATION_RUNTIME_ERROR:
        return STRINGIFY(APPLICATION_RUNTIME_ERROR);
    case APPLICATION_INVALID_ARGUMENT:
        return STRINGIFY(APPLICATION_INVALID_ARGUMENT);
    case APPLICATION_DATA_CORRUPTED:
        return STRINGIFY(APPLICATION_DATA_CORRUPTED);
    case APPLICATION_BAD_OPERATION:
        return STRINGIFY(APPLICATION_BAD_OPERATION);
    case APPLICATION_OVERFLOW:
        return STRINGIFY(APPLICATION_OVERFLOW);
    case APPLICATION_LIMIT_EXCEEDED:
        return STRINGIFY(APPLICATION_LIMIT_EXCEEDED);
    case APPLICATION_UNSUPPORTED_FILE:
        return STRINGIFY(APPLICATION_UNSUPPORTED_FILE);
    case APPLICATION_FILE_OPEN_ERROR:
        return STRINGIFY(APPLICATION_FILE_OPEN_ERROR);
    case APPLICATION_FILE_READ_ERROR:
        return STRINGIFY(APPLICATION_FILE_READ_ERROR);
    case APPLICATION_UNDEFINED_ERROR:
        return STRINGIFY(APPLICATION_UNDEFINED_ERROR);
    default:
        return STRINGIFY(APPLICATION_UNDEFINED_ERROR);
    }
}

application_result_t application_result_convert_subsystem_allocator(subsystem_allocator_result_t result_) {
    switch(result_) {
    case SUBSYSTEM_ALLOCATOR_SUCCESS:
        return APPLICATION_SUCCESS;
    case SUBSYSTEM_ALLOCATOR_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case SUBSYSTEM_ALLOCATOR_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case SUBSYSTEM_ALLOCATOR_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case SUBSYSTEM_ALLOCATOR_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case SUBSYSTEM_ALLOCATOR_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case SUBSYSTEM_ALLOCATOR_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case SUBSYSTEM_ALLOCATOR_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_platform_system(platform_system_result_t result_) {
    switch(result_) {
    case PLATFORM_SYSTEM_SUCCESS:
        return APPLICATION_SUCCESS;
    case PLATFORM_SYSTEM_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case PLATFORM_SYSTEM_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case PLATFORM_SYSTEM_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case PLATFORM_SYSTEM_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case PLATFORM_SYSTEM_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case PLATFORM_SYSTEM_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case PLATFORM_SYSTEM_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case PLATFORM_SYSTEM_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_ring_queue(ring_queue_result_t result_) {
    switch(result_) {
    case RING_QUEUE_SUCCESS:
        return APPLICATION_SUCCESS;
    case RING_QUEUE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RING_QUEUE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RING_QUEUE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RING_QUEUE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    case RING_QUEUE_EMPTY:
        return APPLICATION_RUNTIME_ERROR;   // リングキュー空はRuntime errorに変換
    case RING_QUEUE_OVERFLOW:
        return APPLICATION_RUNTIME_ERROR;   // オーバーフローもRuntime errorに変換
    case RING_QUEUE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RING_QUEUE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RING_QUEUE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_camera_registry(camera_registry_result_t result_) {
    switch(result_) {
    case CAMERA_REGISTRY_SUCCESS:
        return APPLICATION_SUCCESS;
    case CAMERA_REGISTRY_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_REGISTRY_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case CAMERA_REGISTRY_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case CAMERA_REGISTRY_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_REGISTRY_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_REGISTRY_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_REGISTRY_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case CAMERA_REGISTRY_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_camera(camera_result_t result_) {
    switch(result_) {
    case CAMERA_SUCCESS:
        return APPLICATION_SUCCESS;
    case CAMERA_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case CAMERA_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case CAMERA_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case CAMERA_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case CAMERA_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case CAMERA_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case CAMERA_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_fs_path(fs_path_result_t result_) {
    switch(result_) {
    case FS_PATH_SUCCESS:
        return APPLICATION_SUCCESS;
    case FS_PATH_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case FS_PATH_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case FS_PATH_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case FS_PATH_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case FS_PATH_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case FS_PATH_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case FS_PATH_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case FS_PATH_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_general_allocator(general_allocator_result_t result_) {
    switch(result_) {
    case GENERAL_ALLOCATOR_SUCCESS:
        return APPLICATION_SUCCESS;
    case GENERAL_ALLOCATOR_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case GENERAL_ALLOCATOR_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case GENERAL_ALLOCATOR_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case GENERAL_ALLOCATOR_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case GENERAL_ALLOCATOR_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case GENERAL_ALLOCATOR_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case GENERAL_ALLOCATOR_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_renderer_frontend(renderer_frontend_result_t result_) {
    switch(result_) {
    case RENDERER_FRONTEND_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDERER_FRONTEND_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDERER_FRONTEND_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_FRONTEND_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RENDERER_FRONTEND_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDERER_FRONTEND_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDERER_FRONTEND_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDERER_FRONTEND_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDERER_FRONTEND_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_renderer_backend(renderer_backend_result_t result_) {
    switch(result_) {
    case RENDERER_BACKEND_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDERER_BACKEND_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDERER_BACKEND_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_BACKEND_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDERER_BACKEND_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDERER_BACKEND_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDERER_BACKEND_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDERER_BACKEND_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RENDERER_BACKEND_SHADER_COMPILE_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_BACKEND_SHADER_LINK_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDERER_BACKEND_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_mesh(mesh_result_t result_) {
    switch(result_) {
    case MESH_SUCCESS:
        return APPLICATION_SUCCESS;
    case MESH_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case MESH_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MESH_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MESH_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case MESH_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case MESH_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case MESH_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case MESH_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_model_framework(model_framework_result_t result_) {
    switch(result_) {
    case MODEL_FRAMEWORK_SUCCESS:
        return APPLICATION_SUCCESS;
    case MODEL_FRAMEWORK_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case MODEL_FRAMEWORK_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case MODEL_FRAMEWORK_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case MODEL_FRAMEWORK_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case MODEL_FRAMEWORK_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case MODEL_FRAMEWORK_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case MODEL_FRAMEWORK_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case MODEL_FRAMEWORK_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_geometry(geometry_result_t result_) {
    switch(result_) {
    case GEOMETRY_SUCCESS:
        return APPLICATION_SUCCESS;
    case GEOMETRY_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case GEOMETRY_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case GEOMETRY_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case GEOMETRY_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case GEOMETRY_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case GEOMETRY_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case GEOMETRY_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case GEOMETRY_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_render_packet(render_packet_result_t result_) {
    switch(result_) {
    case RENDER_PACKET_SUCCESS:
        return APPLICATION_SUCCESS;
    case RENDER_PACKET_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case RENDER_PACKET_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case RENDER_PACKET_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case RENDER_PACKET_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case RENDER_PACKET_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case RENDER_PACKET_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case RENDER_PACKET_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case RENDER_PACKET_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}

application_result_t application_result_convert_scene(scene_result_t result_) {
    switch(result_) {
    case SCENE_SUCCESS:
        return APPLICATION_SUCCESS;
    case SCENE_NO_MEMORY:
        return APPLICATION_NO_MEMORY;
    case SCENE_RUNTIME_ERROR:
        return APPLICATION_RUNTIME_ERROR;
    case SCENE_INVALID_ARGUMENT:
        return APPLICATION_INVALID_ARGUMENT;
    case SCENE_DATA_CORRUPTED:
        return APPLICATION_DATA_CORRUPTED;
    case SCENE_BAD_OPERATION:
        return APPLICATION_BAD_OPERATION;
    case SCENE_OVERFLOW:
        return APPLICATION_OVERFLOW;
    case SCENE_LIMIT_EXCEEDED:
        return APPLICATION_LIMIT_EXCEEDED;
    case SCENE_UNDEFINED_ERROR:
        return APPLICATION_UNDEFINED_ERROR;
    default:
        return APPLICATION_UNDEFINED_ERROR;
    }
}
