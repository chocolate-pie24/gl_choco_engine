// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file ui_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 *
 * @brief UI描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIを提供する
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @date 2026-06-30
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"

typedef struct vbo_manager vbo_manager_t;
typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t; /**< UI描画用ジオメトリレジストリのopaque型 */

resource_pipeline_result_t ui_mesh_geometry_pipeline_import_from_file(vbo_manager_t* vbo_manager_, ui_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_);

resource_pipeline_result_t ui_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, ui_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
