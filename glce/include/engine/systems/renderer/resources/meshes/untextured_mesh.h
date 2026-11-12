// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UNTEXTURED_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_UNTEXTURED_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * untextured_mesh_config_t
 *
 * 各文字列はBorrowed Pointerであり、Configは所有しない。
 * 呼び出し側は、Configの利用期間中、参照先文字列の
 * Lifetimeを保証する。
 *
 * extensionがNULLの場合、拡張子なしを意味する。
 */
typedef struct untextured_mesh_config {
    const char* geometry_subdirectory;  // assetsからの相対パス
    const char* geometry_resource_name;
    const char* geometry_extension;

    const char* material_subdirectory;  // assetsからの相対パス
    const char* material_resource_name;
    const char* material_extension;
} untextured_mesh_config_t;

typedef struct untextured_mesh {
    uint16_t geometry_id;
    uint16_t material_id;
} untextured_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
