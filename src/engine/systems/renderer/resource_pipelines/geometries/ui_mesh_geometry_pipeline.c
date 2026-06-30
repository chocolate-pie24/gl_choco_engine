#include "engine/systems/renderer/resource_pipelines/geometries/ui_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/config_loaders/ui_geom_config_loader.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

#include "engine/systems/renderer/renderer_core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"

#include "engine/systems/renderer/renderer_resources/shaders/ui_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/ui_mesh_geometry_registry.h"

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_err_utils.h"

// append成功後の後続処理で失敗した場合、shader_に追加された頂点データは巻き戻されない
resource_pipeline_result_t ui_mesh_geometry_pipeline_import_from_file(const renderer_backend_context_t* backend_context_, ui_mesh_shader_t* shader_, ui_mesh_geometry_registry_t* geometry_registry_, const char* name_, int16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    renderer_result_t ret_renderer = RENDERER_INVALID_ARGUMENT;
    resource_registry_result_t ret_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    ui_geom_config_t ui_geometry_config = { 0 };
    ui_mesh_geometry_t* geometry = NULL;
    ui_vertex_t ui_vertex[6] = { 0 };

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

    ret_renderer = ui_mesh_shader_vertex_buffer_append(backend_context_, shader_, sizeof(ui_vertex_t) * vertex_count, ui_vertex, &vertex_offset);
    if(RENDERER_SUCCESS != ret_renderer) {
        ret = resource_pipeline_rslt_convert_renderer(ret_renderer);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=vertex_buffer_append_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_count);
        goto cleanup;
    }

    ret_registry = ui_mesh_geometry_registry_register(geometry_registry_, geometry, vertex_offset, &tmp_geometry_id);
    if(RESOURCE_REGISTRY_SUCCESS != ret_registry) {
        ret = resource_pipeline_rslt_convert_resource_registry(ret_registry);
        ERROR_MESSAGE("ui_mesh_geometry_pipeline_import_from_file(%s) - Failed to import ui mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_offset=%zu, vertex_count=%zu", resource_pipeline_rslt_to_str(ret), name_, vertex_offset, vertex_count);
        goto cleanup;
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    ui_mesh_geometry_destroy(&geometry);
    return ret;
}

resource_pipeline_result_t ui_mesh_geometry_pipeline_release(int16_t geometry_id_) {
    // TODO: VBO FreeList + releaseができたら実装する
    ERROR_MESSAGE("ui_mesh_geometry_pipeline_release(%s) - Failed to release ui mesh geometry. reason=not_implemented, geometry_id=%d, vertex_buffer_release=not_supported", resource_pipeline_rslt_to_str(RESOURCE_PIPELINE_RUNTIME_ERROR), geometry_id_);
    return RESOURCE_PIPELINE_RUNTIME_ERROR;
}
