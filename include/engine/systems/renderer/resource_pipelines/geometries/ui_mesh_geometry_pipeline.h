#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_PIPELINES_GEOMETRIES_UI_MESH_GEOMETRY_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"

typedef struct renderer_backend_context renderer_backend_context_t;   /**< Renderer Backend Contextのopaque型 */
typedef struct ui_mesh_shader ui_mesh_shader_t;                       /**< UI描画用シェーダーリソースのopaque型 */
typedef struct ui_mesh_geometry_registry ui_mesh_geometry_registry_t; /**< UI描画用ジオメトリレジストリのopaque型 */

resource_pipeline_result_t ui_mesh_geometry_pipeline_import_from_size(const renderer_backend_context_t* backend_context_, ui_mesh_shader_t* shader_, ui_mesh_geometry_registry_t* geometry_registry_, const char* name_, uint16_t icon_width_, uint16_t icon_height_, int16_t* out_geometry_id_);

resource_pipeline_result_t ui_mesh_geometry_pipeline_release(int16_t geometry_id_);

#ifdef __cplusplus
}
#endif
#endif
