// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_POINT_MESH_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_MESHES_POINT_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*
 * point_mesh_config_t
 *
 * 各文字列はBorrowed Pointerであり、Configは所有しない。
 * 呼び出し側は、Configの利用期間中、参照先文字列の
 * Lifetimeを保証する。
 *
 * extensionがNULLの場合、拡張子なしを意味する。
 */
typedef struct point_mesh_config {
    const char* geometry_subdirectory;  // assetsからの相対パス
    const char* geometry_resource_name;
    const char* geometry_extension;
} point_mesh_config_t;

typedef struct point_mesh {
    uint16_t geometry_id;
} point_mesh_t;

#ifdef __cplusplus
}
#endif
#endif
