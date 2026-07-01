/** @ingroup renderer
 *
 * @file line_mesh_geometry_pipeline.c
 * @author chocolate-pie24
 *
 * @brief line_mesh用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIの実装
 *
 * @note 本pipelineはCPU側ジオメトリリソース生成、shader resourceへの頂点転送、geometry registryへの登録を一連の手順として実行する
 * @note GPU頂点バッファ自体はshader resourceが所有し、本pipelineは所有しない
 *
 * @version 0.1
 * @date 2026-06-30
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include "engine/systems/renderer/resource_pipelines/geometries/line_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/geometry_primitive/aabb_3d.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/geometry/line_mesh_geometry.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/line_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/line_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_vertices(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const line_vertex_t* vertices_, size_t vertex_count_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;

    line_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 < vertex_count_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_vertices", "vertex_count_")

    ret_resource = line_mesh_geometry_create_from_vertices(name_, vertex_count_, vertices_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }
    vertex_array_size = sizeof(line_vertex_t) * vertex_count_;

    // line_mesh_geometry_create_from_vertices()が成功しているのでオーバーフローチェックは不要
    ret_renderer = line_mesh_shader_vertex_buffer_append(backend_context_, shader_, vertex_array_size, vertices_, &vertex_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count_);
        goto cleanup;
    }

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, geometry, vertex_offset, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_vertices(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count_);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

/**
 * @brief AABB(3D)を使用して線分描画用ジオメトリの生成、GPU頂点バッファへの転送、描画範囲のレジストリ登録を行う
 *
 * @note VBOへのappend成功後の後続処理で失敗した場合、shader_に追加された頂点データは巻き戻されない
 *
 * @param[in] backend_context_ Renderer Backend Context構造体インスタンスへのポインタ
 * @param[in,out] shader_ 線分描画用シェーダーリソース構造体インスタンスへのポインタ
 * @param[in,out] geometry_registry_ 線分描画用ジオメトリレジストリ構造体インスタンスへのポインタ
 * @param[in] name_ ジオメトリ名称
 * @param[in] aabb_ 線分ジオメトリに変換するAABB(3D)
 * @param[out] out_geometry_id_ ジオメトリレジストリ内でのジオメトリ識別子
 *
 * @retval RESOURCE_PIPELINE_INVALID_ARGUMENT 以下のいずれか
 * - backend_context_ == NULL
 * - shader_ == NULL
 * - geometry_registry_ == NULL
 * - name_ == NULL
 * - name_が空文字列
 * - out_geometry_id_ == NULL
 * - aabb_ == NULL
 * @retval RESOURCE_PIPELINE_LIMIT_EXCEEDED 以下のいずれか
 * - メモリシステム使用可能範囲上限超過
 * - 
 * @retval RESOURCE_PIPELINE_BAD_OPERATION 以下のいずれか
 * - メモリシステム未初期化
 * - aabb_が不正
 * @retval RESOURCE_PIPELINE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_PIPELINE_OVERFLOW 処理過程でオーバーフローが発生
 * @retval RESOURCE_DATA_CORRUPTED 生成したgeometryにデータ不整合が発生
 * @retval RESOURCE_BAD_OPERATION 何らかの理由で生成したgeometryが未初期化となり、後続APIに渡った
 */
// line_mesh_shader_vertex_buffer_append
// RENDERER_INVALID_ARGUMENT 以下のいずれか - backend_context_ == NULL - line_mesh_shader_ == NULL - write_data_ == NULL - size_ == 0 - out_vertex_offset_ == NULL - size_がsizeof(line_vertex_t) x 2の倍数ではない
// RENDERER_LIMIT_EXCEEDED 転送サイズ後のcurrent_buffer_offsetがSIZE_MAXを超過
// RENDERER_BAD_OPERATION 以下のいずれか - VBO未初期化 - 転送後にバーテックスバッファサイズを超過 - backend_context_が未初期化
// RENDERER_SUCCESS 処理に成功し、正常終了

// line_mesh_geometry_registry_register
// RESOURCE_REGISTRY_INVALID_ARGUMENT 以下のいずれか - registry_ == NULL - geometry_ == NULL - out_geometry_id_ == NULL - geometry_が未初期化でジオメトリ名称が取得できない
// RESOURCE_REGISTRY_DATA_CORRUPTED 以下のいずれか - registry_の内部データ不整合が発生している - geometry_の内部データ不整合が発生している
// RESOURCE_REGISTRY_BAD_OPERATION 以下のいずれか - geometry_のジオメトリ名称が既にregistry_に登録されている - メモリシステム未初期化
// RESOURCE_REGISTRY_NO_MEMORY メモリ確保失敗
// RESOURCE_REGISTRY_LIMIT_EXCEEDED 以下のいずれか - メモリシステム使用可能範囲上限超過 - registry_に空きスロットが見つからない
// RESOURCE_REGISTRY_SUCCESS 処理に成功し、正常終了
resource_pipeline_result_t line_mesh_geometry_pipeline_import_from_aabb(const renderer_backend_context_t* backend_context_, line_mesh_shader_t* shader_, line_mesh_geometry_registry_t* geometry_registry_, const char* name_, const aabb_3d_t* aabb_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t vertex_count = 0;
    size_t vertex_offset = 0;
    size_t vertex_array_size = 0;
    int16_t tmp_geometry_id = 0;

    line_mesh_geometry_t* geometry = NULL;
    const line_vertex_t* vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(aabb_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "line_mesh_geometry_pipeline_import_from_aabb", "aabb_")

    ret_resource = line_mesh_geometry_create_from_aabbs(name_, 1, aabb_, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_create_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    ret_resource = line_mesh_geometry_vertex_count_get(geometry, &vertex_count);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertex_count_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    ret_resource = line_mesh_geometry_vertices_get(geometry, &vertices);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertices_get_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }
    vertex_array_size = sizeof(line_vertex_t) * vertex_count;   // 単体のAABBなのでオーバーフローチェックは不要

    ret_renderer = line_mesh_shader_vertex_buffer_append(backend_context_, shader_, vertex_array_size, vertices, &vertex_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }

    // NOTE: 一時的にverticesが2つ分必要なので、deep copyではなくmoveを検討しても良い
    ret_registry = line_mesh_geometry_registry_register(geometry_registry_, geometry, vertex_offset, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("line_mesh_geometry_pipeline_import_from_aabb(%s) - Failed to import line mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    line_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t line_mesh_geometry_pipeline_release(int16_t geometry_id_) {
    // TODO: VBO FreeList + releaseができたら実装する
    ERROR_MESSAGE("line_mesh_geometry_pipeline_release(%s) - Failed to release line mesh geometry. reason=not_implemented, geometry_id=%d, vertex_buffer_release=not_supported", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_RUNTIME_ERROR), geometry_id_);
    return RESOURCE_PIPELINE_RUNTIME_ERROR;
}
