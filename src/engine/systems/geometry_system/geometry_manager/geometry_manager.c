#include <stdint.h>
#include <stddef.h>

#include "engine/systems/geometry_system/geometry_manager/geometry_manager.h"

#include "engine/systems/geometry_system/geometry_system_core/geometry_system_types.h"
#include "engine/systems/geometry_system/geometry_system_core/geometry_system_err_utils.h"

#include "engine/resource/geometry/line_mesh_geometry.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"
#include "engine/resource/geometry/point_mesh_geometry.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

struct geometry_system {
    geometry_system_config_t geometry_system_config;

    lit_mesh_geometry_t** lit_mesh_geometries;
    line_mesh_geometry_t** line_mesh_geometries;
    point_mesh_geometry_t** point_mesh_geometries;
    ui_mesh_geometry_t** ui_mesh_geometries;

    size_t* lit_mesh_geometry_vertex_offset;
    size_t* line_mesh_geometry_vertex_offset;
    size_t* point_mesh_geometry_vertex_offset;
    size_t* ui_mesh_geometry_vertex_offset;
};

geometry_system_result_t geometry_system_initialize(const geometry_system_config_t* config_, geometry_system_t** out_geometry_system_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

void geometry_system_deinitialize(geometry_system_t* geometry_system_) {
}

geometry_system_result_t geometry_system_draw_range_get_by_name(geometry_type_t geometry_type_, const char* name_, const geometry_system_t* geometry_system_, size_t* out_vertex_offset_, size_t* out_vertex_count_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

geometry_system_result_t geometry_system_draw_range_get_by_id(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_system_t* geometry_system_, size_t* out_vertex_offset_, size_t* out_vertex_count_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

// geometry_をgeometry_system_へdeep copy
geometry_system_result_t geometry_system_geometry_register(geometry_type_t geometry_type_, const void* geometry_, geometry_system_t* geometry_system_, int16_t* out_geometry_id_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

geometry_system_result_t geometry_system_geometry_unregister_by_name(geometry_type_t geometry_type_, const char* name_, geometry_system_t* geometry_system_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

geometry_system_result_t geometry_system_geometry_unregister_by_id(geometry_type_t geometry_type_, int16_t geometry_id_, geometry_system_t* geometry_system_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}

geometry_system_result_t geometry_system_geometry_id_get(geometry_type_t geometry_type_, const char* name_, const geometry_system_t* geometry_system_, int16_t* out_geometry_id_) {
    return GEOMETRY_SYSTEM_SUCCESS;
}
