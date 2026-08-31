// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file line_mesh_geometry_pipeline.h
 * @author chocolate-pie24
 *
 * @brief line_mesh用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIを提供する
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @date 2026-06-30
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_LINE_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;         /**< Renderer Backend Contextのopaque型 */
typedef struct line_mesh_shader line_mesh_shader_t;                         /**< 線分描画用シェーダーリソースのopaque型 */
typedef struct line_mesh_geometry_registry line_mesh_geometry_registry_t;   /**< 線分描画用ジオメトリレジストリのopaque型 */
typedef struct line_vertex line_vertex_t;
typedef struct aabb_3d aabb_3d_t;

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const line_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_);

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const aabb_3d_t* aabb_, int16_t* out_geometry_id_);

resource_pipeline_result_t line_mesh_geometry_pipeline_release(line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
