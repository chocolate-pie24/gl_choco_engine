/** @ingroup renderer
 *
 * @file ui_mesh_geometry_pipeline.c
 * @author chocolate-pie24
 *
 * @brief UI描画用ジオメトリ入力をGPU頂点バッファへ転送し、描画範囲をレジストリへ登録するpipeline APIの実装
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
#include "engine/systems/renderer/resource_pipelines/geometries/ui_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/config_loaders/ui_geom_config_loader.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/renderer_resources/shaders/ui_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/ui_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

resource_pipeline_result_t ui_mesh_geometry_pipeline_import_from_file(const renderer_backend_context_t* backend_context_, ui_mesh_shader_t* shader_, ui_mesh_geometry_registry_t* geometry_registry_, const char* name_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    ui_geom_config_t ui_geometry_config = { 0 };
    ui_mesh_geometry_t* geometry = NULL;
    ui_vertex_t ui_vertex[6] = { 0 };
    vertex_buffer_range_t tmp_buffer_range = { 0 };
    bool vbo_written = false;

    const size_t vertex_count = 6;
    size_t vertex_offset = 0;
    int16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_import_from_file", "out_geometry_id_")

    ret_resource = ui_geom_config_loader_load(name_, &ui_geometry_config);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=ui_geom_config_load_failed, geometry_name='%s'", resource_pipeline_rslt_to_str(ret), name_);
        goto cleanup;
    }

    ui_vertex[0].position = vec2f_initialize(0.0f, 0.0f);
    ui_vertex[1].position = vec2f_initialize(ui_geometry_config.icon_width, 0.0f);
    ui_vertex[2].position = vec2f_initialize(ui_geometry_config.icon_width, ui_geometry_config.icon_height);

    ui_vertex[3].position = vec2f_initialize(0.0f, 0.0f);
    ui_vertex[4].position = vec2f_initialize(ui_geometry_config.icon_width, ui_geometry_config.icon_height);
    ui_vertex[5].position = vec2f_initialize(0.0f, ui_geometry_config.icon_height);

    ui_vertex[0].tex_coord = vec2f_initialize(0.0f, 1.0f);
    ui_vertex[1].tex_coord = vec2f_initialize(1.0f, 1.0f);
    ui_vertex[2].tex_coord = vec2f_initialize(1.0f, 0.0f);

    ui_vertex[3].tex_coord = vec2f_initialize(0.0f, 1.0f);
    ui_vertex[4].tex_coord = vec2f_initialize(1.0f, 0.0f);
    ui_vertex[5].tex_coord = vec2f_initialize(0.0f, 0.0f);

    ret_resource = ui_mesh_geometry_create_from_vertices(name_, vertex_count, ui_vertex, &geometry);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = resource_pipeline_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }

    ret_shader = ui_mesh_shader_vbo_write(backend_context_, shader_, vertex_count, ui_vertex, &tmp_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, 6);
        goto cleanup;
    }
    vbo_written = true;

    ret_registry = ui_mesh_geometry_registry_register(geometry_registry_, geometry, &tmp_buffer_range, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_shader = ui_mesh_shader_vbo_free(shader_, &tmp_buffer_range);
        if(SHADER_SUCCESS != ret_shader) {
            // NOTE: ui_mesh_shader_vbo_freeが失敗した場合はbuffer_managerにデータ不整合が発生しているため、
            // ui_mesh_geometry_pipeline_import_from_file失敗理由に関わらず、重大エラーのDATA_CORRUPTEDを返す
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - ui mesh geometry import failed.", resource_pipeline_rslt_to_str(ret));
        }
    }
    ui_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t ui_mesh_geometry_pipeline_release(ui_mesh_shader_t* shader_, ui_mesh_geometry_registry_t* geometry_registry_, int16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    vertex_buffer_range_t vertex_buffer_range = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_release", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "ui_mesh_geometry_pipeline_release", "geometry_registry_")

    ret_registry = ui_mesh_geometry_registry_vertex_buffer_range_get(geometry_registry_, geometry_id_, &vertex_buffer_range);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - ui_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - ui_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    // unregisterに失敗した場合はgeometry_registry_は不変となる。そのため、vbo_freeの後でunregisterに失敗するとgeometry_registry_に解放済みallocationへの参照が残る。よってvbo_freeの前で実行する
    ret_registry = ui_mesh_geometry_registry_unregister(geometry_registry_, geometry_id_);
    if(RESOURCE_REGISTRY_INVALID_ARGUMENT == ret_registry || RESOURCE_REGISTRY_BAD_OPERATION == ret_registry) { // geometry_idが異常
        ret = RESOURCE_PIPELINE_RUNTIME_ERROR;
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - ui_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    } else if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - ui_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = ui_mesh_shader_vbo_free(shader_, &vertex_buffer_range);
    if(SHADER_SUCCESS != ret_shader) {
        ret = resource_pipeline_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - ui_mesh_geometry_pipeline_release failed.", resource_pipeline_rslt_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
