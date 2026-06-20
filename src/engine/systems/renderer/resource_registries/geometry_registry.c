#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/geometry_registry.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/geometry/line_mesh_geometry.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"
#include "engine/resource/geometry/point_mesh_geometry.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

struct geometry_registry {
    geometry_registry_config_t geometry_registry_config;

    // CPU resources
    lit_mesh_geometry_t** lit_mesh_geometries;
    line_mesh_geometry_t** line_mesh_geometries;
    point_mesh_geometry_t** point_mesh_geometries;
    ui_mesh_geometry_t** ui_mesh_geometries;

    // GPU resources
    size_t* lit_mesh_geometry_vertex_offset;
    size_t* line_mesh_geometry_vertex_offset;
    size_t* point_mesh_geometry_vertex_offset;
    size_t* ui_mesh_geometry_vertex_offset;
};

// static bool geometry_type_valid_check(geometry_type_t geometry_type_);
// static bool geometry_id_valid_check(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_registry_t* geometry_registry_);
// static bool geometry_registry_internal_state_check(const geometry_registry_t* geometry_registry_);
// static bool lit_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_);
// static bool line_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_);
// static bool point_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_);
// static bool ui_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_);

// resource_registry_result_t geometry_registry_initialize(const geometry_registry_config_t* config_, linear_alloc_t* allocator_, geometry_registry_t** out_geometry_registry_) {
//     resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
//     linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

//     geometry_registry_t* tmp_registry = NULL;
//     lit_mesh_geometry_t** tmp_lit_mesh_geometries = NULL;
//     line_mesh_geometry_t** tmp_line_mesh_geometries = NULL;
//     point_mesh_geometry_t** tmp_point_mesh_geometries = NULL;
//     ui_mesh_geometry_t** tmp_ui_mesh_geometries = NULL;

//     size_t* tmp_lit_mesh_geometry_vertex_offset = NULL;
//     size_t* tmp_line_mesh_geometry_vertex_offset = NULL;
//     size_t* tmp_point_mesh_geometry_vertex_offset = NULL;
//     size_t* tmp_ui_mesh_geometry_vertex_offset = NULL;

//     // config_内の各Geometry最大数は0を許容する
//     IF_ARG_NULL_GOTO_CLEANUP(config_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_initialize", "config_")
//     IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_initialize", "allocator_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_initialize", "out_geometry_registry_")
//     IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_initialize", "*out_geometry_registry_")

//     // geometry_registry_tメモリ確保
//     ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(geometry_registry_t), alignof(geometry_registry_t), (void**)&tmp_registry);
//     if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//         ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//         ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for geometry_registry_t instance.", resource_registry_rslt_to_str(ret));
//         goto cleanup;
//     }
//     memset(tmp_registry, 0, sizeof(geometry_registry_t));

//     if(0 != config_->max_line_mesh_geometry_count) {
//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(line_mesh_geometry_t*) * config_->max_line_mesh_geometry_count, alignof(line_mesh_geometry_t*), (void**)&tmp_line_mesh_geometries);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for line_mesh_geometry_t* array instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }

//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_line_mesh_geometry_count, alignof(size_t*), (void**)&tmp_line_mesh_geometry_vertex_offset);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for vertex offset array(line mesh) instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//     }

//     if(0 != config_->max_lit_mesh_geometry_count) {
//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(lit_mesh_geometry_t*) * config_->max_lit_mesh_geometry_count, alignof(lit_mesh_geometry_t*), (void**)&tmp_lit_mesh_geometries);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for lit_mesh_geometry_t* array instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }

//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_lit_mesh_geometry_count, alignof(size_t*), (void**)&tmp_lit_mesh_geometry_vertex_offset);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for vertex offset array(lit mesh) instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//     }

//     if(0 != config_->max_point_mesh_geometry_count) {
//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(point_mesh_geometry_t*) * config_->max_point_mesh_geometry_count, alignof(point_mesh_geometry_t*), (void**)&tmp_point_mesh_geometries);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for point_mesh_geometry_t* array instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }

//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_point_mesh_geometry_count, alignof(size_t*), (void**)&tmp_point_mesh_geometry_vertex_offset);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for vertex offset array(point mesh) instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//     }

//     if(0 != config_->max_ui_mesh_geometry_count) {
//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(ui_mesh_geometry_t*) * config_->max_ui_mesh_geometry_count, alignof(ui_mesh_geometry_t*), (void**)&tmp_ui_mesh_geometries);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for ui_mesh_geometry_t* array instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }

//         ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_ui_mesh_geometry_count, alignof(size_t*), (void**)&tmp_ui_mesh_geometry_vertex_offset);
//         if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
//             ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
//             ERROR_MESSAGE("geometry_registry_initialize(%s) - Failed to allocate memory for vertex offset array(ui mesh) instance.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//     }

//     tmp_registry->geometry_registry_config = *config_;
//     for(size_t i = 0; i != config_->max_line_mesh_geometry_count; ++i) {
//         tmp_registry->line_mesh_geometries[i] = NULL;
//     }
//     for(size_t i = 0; i != config_->max_lit_mesh_geometry_count; ++i) {
//         tmp_registry->lit_mesh_geometries[i] = NULL;
//     }
//     for(size_t i = 0; i != config_->max_point_mesh_geometry_count; ++i) {
//         tmp_registry->point_mesh_geometries[i] = NULL;
//     }
//     for(size_t i = 0; i != config_->max_ui_mesh_geometry_count; ++i) {
//         tmp_registry->ui_mesh_geometries[i] = NULL;
//     }

//     tmp_registry->line_mesh_geometries = tmp_line_mesh_geometries;
//     tmp_registry->lit_mesh_geometries = tmp_lit_mesh_geometries;
//     tmp_registry->point_mesh_geometries = tmp_point_mesh_geometries;
//     tmp_registry->ui_mesh_geometries = tmp_ui_mesh_geometries;

//     tmp_registry->line_mesh_geometry_vertex_offset = tmp_line_mesh_geometry_vertex_offset;
//     tmp_registry->lit_mesh_geometry_vertex_offset = tmp_lit_mesh_geometry_vertex_offset;
//     tmp_registry->point_mesh_geometry_vertex_offset = tmp_point_mesh_geometry_vertex_offset;
//     tmp_registry->ui_mesh_geometry_vertex_offset = tmp_ui_mesh_geometry_vertex_offset;

//     *out_geometry_registry_ = tmp_registry;

//     ret = RESOURCE_REGISTRY_SUCCESS;

// cleanup:
//     // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
//     return ret;
// }

// void geometry_registry_deinitialize(geometry_registry_t* geometry_registry_) {
//     if(NULL == geometry_registry_) {
//         return;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count; ++i) {
//         line_mesh_geometry_destroy(&geometry_registry_->line_mesh_geometries[i]);
//         geometry_registry_->line_mesh_geometry_vertex_offset[i] = 0;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count; ++i) {
//         lit_mesh_geometry_destroy(&geometry_registry_->lit_mesh_geometries[i]);
//         geometry_registry_->lit_mesh_geometry_vertex_offset[i] = 0;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count; ++i) {
//         point_mesh_geometry_destroy(&geometry_registry_->point_mesh_geometries[i]);
//         geometry_registry_->point_mesh_geometry_vertex_offset[i] = 0;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count; ++i) {
//         ui_mesh_geometry_destroy(&geometry_registry_->ui_mesh_geometries[i]);
//         geometry_registry_->ui_mesh_geometry_vertex_offset[i] = 0;
//     }
// }

// bool geometry_registry_geometry_find(geometry_type_t geometry_type_, const char* name_, const geometry_registry_t* geometry_registry_) {
//     int16_t tmp_id = 0;

//     if(!geometry_type_valid_check(geometry_type_)) {
//         ERROR_MESSAGE("geometry_registry_geometry_find - Invalid geometry type.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
//         return false;
//     }
//     if(NULL == name_) {
//         // これは場合によっては起こりうる(かも)ので、メッセージは出さない
//         return false;
//     }
//     if(NULL == geometry_registry_) {
//         // これは場合によっては起こりうる(かも)ので、メッセージは出さない
//         return false;
//     }
//     if(!geometry_registry_internal_state_check(geometry_registry_)) {
//         ERROR_MESSAGE("geometry_registry_geometry_find(%s) - geometry_registry_t internal state is inconsistent.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
//         return false;
//     }

//     if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_ && line_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         return true;
//     } else if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_ && lit_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         return true;
//     } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_ && point_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         return true;
//     } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_ && ui_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         return true;
//     } else {
//         ERROR_MESSAGE("geometry_registry_geometry_find - missed implementing...");
//         return false;
//     }
// }

// resource_registry_result_t geometry_registry_geometry_id_get(geometry_type_t geometry_type_, const char* name_, const geometry_registry_t* geometry_registry_, int16_t* out_geometry_id_) {
//     resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

//     int16_t tmp_id = INVALID_GEOMETRY_ID;

//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_type_valid_check(geometry_type_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_id_get", "geometry_type_")
//     IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_id_get", "name_")
//     IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_id_get", "geometry_registry_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_id_get", "out_geometry_id_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_registry_internal_state_check(geometry_registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "geometry_registry_geometry_id_get", "geometry_registry_")

//     if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_ && !line_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         ret = RESOURCE_REGISTRY_BAD_OPERATION;
//         ERROR_MESSAGE("geometry_registry_geometry_id_get(%s) - Provided line mesh geometry: '%s' not found.", resource_registry_rslt_to_str(ret), name_);
//         goto cleanup;
//     } else if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_ && !lit_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         ret = RESOURCE_REGISTRY_BAD_OPERATION;
//         ERROR_MESSAGE("geometry_registry_geometry_id_get(%s) - Provided lit mesh geometry: '%s' not found.", resource_registry_rslt_to_str(ret), name_);
//         goto cleanup;
//     } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_ && !point_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         ret = RESOURCE_REGISTRY_BAD_OPERATION;
//         ERROR_MESSAGE("geometry_registry_geometry_id_get(%s) - Provided point mesh geometry: '%s' not found.", resource_registry_rslt_to_str(ret), name_);
//         goto cleanup;
//     } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_ && !ui_mesh_geometry_find(name_, geometry_registry_, &tmp_id)) {
//         ret = RESOURCE_REGISTRY_BAD_OPERATION;
//         ERROR_MESSAGE("geometry_registry_geometry_id_get(%s) - Provided ui mesh geometry: '%s' not found.", resource_registry_rslt_to_str(ret), name_);
//         goto cleanup;
//     } else {
//         ret = RESOURCE_REGISTRY_RUNTIME_ERROR;
//         ERROR_MESSAGE("geometry_registry_geometry_id_get - missed implementing...");
//         goto cleanup;
//     }

//     *out_geometry_id_ = tmp_id;

//     ret = RESOURCE_REGISTRY_SUCCESS;

// cleanup:
//     return ret;
// }

// resource_registry_result_t geometry_registry_draw_range_get(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_registry_t* geometry_registry_, size_t* out_vertex_offset_, size_t* out_vertex_count_) {
//     resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
//     resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

//     size_t tmp_count = 0;
//     size_t tmp_offset = 0;

//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_type_valid_check(geometry_type_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_draw_range_get", "geometry_type_")
//     IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_draw_range_get", "geometry_registry_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_vertex_offset_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_draw_range_get", "out_vertex_offset_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_draw_range_get", "out_vertex_count_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_registry_internal_state_check(geometry_registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "geometry_registry_draw_range_get", "geometry_registry_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_id_valid_check(geometry_type_, geometry_id_, geometry_registry_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_draw_range_get", "geometry_id_")

//     if(geometry_type_ == GEOMETRY_TYPE_LINE_MESH_GEOMETRY) {
//         if(NULL == geometry_registry_->line_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Provided line mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         ret_resource = line_mesh_geometry_vertex_count_get(geometry_registry_->line_mesh_geometries[geometry_id_], &tmp_count);
//         if(RESOURCE_SUCCESS != ret_resource) {
//             ret = resource_registry_rslt_convert_resource(ret_resource);
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Failed to get line mesh geometry vertex count.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         tmp_offset = geometry_registry_->line_mesh_geometry_vertex_offset[geometry_id_];
//     } else if(geometry_type_ == GEOMETRY_TYPE_LIT_MESH_GEOMETRY) {
//         if(NULL == geometry_registry_->lit_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Provided lit mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         ret_resource = lit_mesh_geometry_vertex_count_get(geometry_registry_->lit_mesh_geometries[geometry_id_], &tmp_count);
//         if(RESOURCE_SUCCESS != ret_resource) {
//             ret = resource_registry_rslt_convert_resource(ret_resource);
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Failed to get lit mesh geometry vertex count.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         tmp_offset = geometry_registry_->lit_mesh_geometry_vertex_offset[geometry_id_];
//     } else if(geometry_type_ == GEOMETRY_TYPE_POINT_MESH_GEOMETRY) {
//         if(NULL == geometry_registry_->point_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Provided point mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         ret_resource = point_mesh_geometry_vertex_count_get(geometry_registry_->point_mesh_geometries[geometry_id_], &tmp_count);
//         if(RESOURCE_SUCCESS != ret_resource) {
//             ret = resource_registry_rslt_convert_resource(ret_resource);
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Failed to get point mesh geometry vertex count.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         tmp_offset = geometry_registry_->point_mesh_geometry_vertex_offset[geometry_id_];
//     } else if(geometry_type_ == GEOMETRY_TYPE_UI_MESH_GEOMETRY) {
//         if(NULL == geometry_registry_->ui_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Provided ui mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         ret_resource = ui_mesh_geometry_vertex_count_get(geometry_registry_->ui_mesh_geometries[geometry_id_], &tmp_count);
//         if(RESOURCE_SUCCESS != ret_resource) {
//             ret = resource_registry_rslt_convert_resource(ret_resource);
//             ERROR_MESSAGE("geometry_registry_draw_range_get(%s) - Failed to get ui mesh geometry vertex count.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         tmp_offset = geometry_registry_->ui_mesh_geometry_vertex_offset[geometry_id_];
//     } else {
//         ret = RESOURCE_REGISTRY_RUNTIME_ERROR;
//         ERROR_MESSAGE("geometry_registry_draw_range_get - missed implementing...");
//         goto cleanup;
//     }

//     *out_vertex_count_ = tmp_count;
//     *out_vertex_offset_ = tmp_offset;

//     ret = RESOURCE_REGISTRY_SUCCESS;

// cleanup:
//     return ret;
// }

// // geometry_をgeometry_registry_へdeep copy
// // TODO: 重複nameの登録禁止
// resource_registry_result_t geometry_registry_geometry_register(geometry_type_t geometry_type_, const void* geometry_, size_t vertex_offset_, geometry_registry_t* geometry_registry_, int16_t* out_geometry_id_) {
//     resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
//     resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

//     bool found_free_slot = false;

//     line_mesh_geometry_t* src_line_mesh_geometry = NULL;
//     line_mesh_geometry_t* new_line_mesh_geometry = NULL;

//     lit_mesh_geometry_t* src_lit_mesh_geometry = NULL;
//     lit_mesh_geometry_t* new_lit_mesh_geometry = NULL;

//     point_mesh_geometry_t* src_point_mesh_geometry = NULL;
//     point_mesh_geometry_t* new_point_mesh_geometry = NULL;

//     ui_mesh_geometry_t* src_ui_mesh_geometry = NULL;
//     ui_mesh_geometry_t* new_ui_mesh_geometry = NULL;

//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_type_valid_check(geometry_type_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_register", "geometry_type_")
//     IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_register", "geometry_registry_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_registry_internal_state_check(geometry_registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "geometry_registry_geometry_register", "geometry_registry_")
//     IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_register", "geometry_")
//     IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_register", "out_geometry_id_")

//     if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_) {
//         src_line_mesh_geometry = (line_mesh_geometry_t*)geometry_;
//         for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count; ++i) {
//             if(NULL == geometry_registry_->line_mesh_geometries[i]) {
//                 ret_resource = line_mesh_geometry_clone(src_line_mesh_geometry, &new_line_mesh_geometry);
//                 if(RESOURCE_SUCCESS != ret_resource) {
//                     ret = resource_registry_rslt_convert_resource(ret_resource);
//                     ERROR_MESSAGE("geometry_registry_geometry_register(%s) - Failed to clone line mesh geometry.", resource_registry_rslt_to_str(ret));
//                     goto cleanup;
//                 }
//                 found_free_slot = true;
//                 geometry_registry_->line_mesh_geometry_vertex_offset[i] = vertex_offset_;
//                 geometry_registry_->line_mesh_geometries[i] = new_line_mesh_geometry;
//                 *out_geometry_id_ = i;
//                 break;
//             }
//         }
//     } else if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_) {
//         src_lit_mesh_geometry = (lit_mesh_geometry_t*)geometry_;
//         for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count; ++i) {
//             if(NULL == geometry_registry_->lit_mesh_geometries[i]) {
//                 ret_resource = lit_mesh_geometry_clone(src_lit_mesh_geometry, &new_lit_mesh_geometry);
//                 if(RESOURCE_SUCCESS != ret_resource) {
//                     ret = resource_registry_rslt_convert_resource(ret_resource);
//                     ERROR_MESSAGE("geometry_registry_geometry_register(%s) - Failed to clone lit mesh geometry.", resource_registry_rslt_to_str(ret));
//                     goto cleanup;
//                 }
//                 found_free_slot = true;
//                 geometry_registry_->lit_mesh_geometry_vertex_offset[i] = vertex_offset_;
//                 geometry_registry_->lit_mesh_geometries[i] = new_lit_mesh_geometry;
//                 *out_geometry_id_ = i;
//                 break;
//             }
//         }
//     } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_) {
//         src_point_mesh_geometry = (point_mesh_geometry_t*)geometry_;
//         for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count; ++i) {
//             if(NULL == geometry_registry_->point_mesh_geometries[i]) {
//                 ret_resource = point_mesh_geometry_clone(src_point_mesh_geometry, &new_point_mesh_geometry);
//                 if(RESOURCE_SUCCESS != ret_resource) {
//                     ret = resource_registry_rslt_convert_resource(ret_resource);
//                     ERROR_MESSAGE("geometry_registry_geometry_register(%s) - Failed to clone point mesh geometry.", resource_registry_rslt_to_str(ret));
//                     goto cleanup;
//                 }
//                 found_free_slot = true;
//                 geometry_registry_->point_mesh_geometry_vertex_offset[i] = vertex_offset_;
//                 geometry_registry_->point_mesh_geometries[i] = new_point_mesh_geometry;
//                 *out_geometry_id_ = i;
//                 break;
//             }
//         }
//     } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_) {
//         src_ui_mesh_geometry = (ui_mesh_geometry_t*)geometry_;
//         for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count; ++i) {
//             if(NULL == geometry_registry_->ui_mesh_geometries[i]) {
//                 ret_resource = ui_mesh_geometry_clone(src_ui_mesh_geometry, &new_ui_mesh_geometry);
//                 if(RESOURCE_SUCCESS != ret_resource) {
//                     ret = resource_registry_rslt_convert_resource(ret_resource);
//                     ERROR_MESSAGE("geometry_registry_geometry_register(%s) - Failed to clone ui mesh geometry.", resource_registry_rslt_to_str(ret));
//                     goto cleanup;
//                 }
//                 found_free_slot = true;
//                 geometry_registry_->ui_mesh_geometry_vertex_offset[i] = vertex_offset_;
//                 geometry_registry_->ui_mesh_geometries[i] = new_ui_mesh_geometry;
//                 *out_geometry_id_ = i;
//                 break;
//             }
//         }
//     } else {
//         ret = RESOURCE_REGISTRY_RUNTIME_ERROR;
//         ERROR_MESSAGE("geometry_registry_geometry_register - missed implementing...");
//         goto cleanup;
//     }

//     if(!found_free_slot) {
//         ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
//         ERROR_MESSAGE("geometry_registry_geometry_register(%s) - geometry registry free slot not found.", resource_registry_rslt_to_str(ret));
//         goto cleanup;
//     }

//     ret = RESOURCE_REGISTRY_SUCCESS;

// cleanup:
//     return ret;
// }

// resource_registry_result_t geometry_registry_geometry_unregister(geometry_type_t geometry_type_, int16_t geometry_id_, geometry_registry_t* geometry_registry_) {
//     resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
//     resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_type_valid_check(geometry_type_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_unregister", "geometry_type_")
//     IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_unregister", "geometry_registry_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_registry_internal_state_check(geometry_registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "geometry_registry_geometry_unregister", "geometry_registry_")
//     IF_ARG_FALSE_GOTO_CLEANUP(geometry_id_valid_check(geometry_type_, geometry_id_, geometry_registry_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "geometry_registry_geometry_unregister", "geometry_id_")

//     if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_) {
//         if(NULL == geometry_registry_->line_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_geometry_unregister(%s) - Provided line mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         line_mesh_geometry_destroy(&geometry_registry_->line_mesh_geometries[geometry_id_]);
//         geometry_registry_->line_mesh_geometry_vertex_offset[geometry_id_] = 0;
//     } else if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_) {
//         if(NULL == geometry_registry_->lit_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_geometry_unregister(%s) - Provided lit mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         lit_mesh_geometry_destroy(&geometry_registry_->lit_mesh_geometries[geometry_id_]);
//         geometry_registry_->lit_mesh_geometry_vertex_offset[geometry_id_] = 0;
//     } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_) {
//         if(NULL == geometry_registry_->point_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_geometry_unregister(%s) - Provided point mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         point_mesh_geometry_destroy(&geometry_registry_->point_mesh_geometries[geometry_id_]);
//         geometry_registry_->point_mesh_geometry_vertex_offset[geometry_id_] = 0;
//     } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_) {
//         if(NULL == geometry_registry_->ui_mesh_geometries[geometry_id_]) {
//             ret = RESOURCE_REGISTRY_BAD_OPERATION;
//             ERROR_MESSAGE("geometry_registry_geometry_unregister(%s) - Provided ui mesh geometry id is not registered.", resource_registry_rslt_to_str(ret));
//             goto cleanup;
//         }
//         ui_mesh_geometry_destroy(&geometry_registry_->ui_mesh_geometries[geometry_id_]);
//         geometry_registry_->ui_mesh_geometry_vertex_offset[geometry_id_] = 0;
//     } else {
//         ret = RESOURCE_REGISTRY_RUNTIME_ERROR;
//         ERROR_MESSAGE("geometry_registry_geometry_unregister - missed implementing...");
//         goto cleanup;
//     }

//     ret = RESOURCE_REGISTRY_SUCCESS;

// cleanup:
//     return ret;
// }

// static bool geometry_type_valid_check(geometry_type_t geometry_type_) {
//     switch(geometry_type_) {
//     case GEOMETRY_TYPE_LIT_MESH_GEOMETRY:
//         return true;
//     case GEOMETRY_TYPE_LINE_MESH_GEOMETRY:
//         return true;
//     case GEOMETRY_TYPE_POINT_MESH_GEOMETRY:
//         return true;
//     case GEOMETRY_TYPE_UI_MESH_GEOMETRY:
//         return true;
//     default:
//         return false;
//     }
// }

// // 本関数を呼び出す前に以下の条件はチェックしておくこと
// // geometry_type_valid_check(geometry_type_) == true
// // geometry_registry_ != NULL
// // geometry_registry_internal_state_check(geometry_registry_) == true
// static bool geometry_id_valid_check(geometry_type_t geometry_type_, int16_t geometry_id_, const geometry_registry_t* geometry_registry_) {
//     if(geometry_id_ < 0) {
//         return false;
//     }
//     if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_ && geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count <= geometry_id_) {
//         return false;
//     } else if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_ && geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count <= geometry_id_) {
//         return false;
//     } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_ && geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count <= geometry_id_) {
//         return false;
//     } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_ && geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count <= geometry_id_) {
//         return false;
//     }
//     return true;
// }

// static bool geometry_registry_internal_state_check(const geometry_registry_t* geometry_registry_) {
//     if(0 != geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count && (NULL == geometry_registry_->line_mesh_geometries || NULL == geometry_registry_->line_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 != geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count && (NULL == geometry_registry_->lit_mesh_geometries || NULL == geometry_registry_->lit_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 != geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count && (NULL == geometry_registry_->point_mesh_geometries || NULL == geometry_registry_->point_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 != geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count && (NULL == geometry_registry_->ui_mesh_geometries || NULL == geometry_registry_->ui_mesh_geometry_vertex_offset)) {
//         return false;
//     }

//     if(0 == geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count && (NULL != geometry_registry_->line_mesh_geometries || NULL != geometry_registry_->line_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 == geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count && (NULL != geometry_registry_->lit_mesh_geometries || NULL != geometry_registry_->lit_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 == geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count && (NULL != geometry_registry_->point_mesh_geometries || NULL != geometry_registry_->point_mesh_geometry_vertex_offset)) {
//         return false;
//     } else if(0 == geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count && (NULL != geometry_registry_->ui_mesh_geometries || NULL != geometry_registry_->ui_mesh_geometry_vertex_offset)) {
//         return false;
//     }

//     return true;
// }

// // 本関数を呼び出す前に以下の条件はチェックしておくこと
// // name_ != NULL
// // geometry_registry_ != NULL
// // geometry_registry_internal_state_check(geometry_registry_) == true
// // out_index_ != NULL
// static bool lit_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_) {
//     const char* tmp_name = NULL;
//     size_t tmp_slot = 0;
//     bool found = false;

//     if(0 == geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count) {
//         return false;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_lit_mesh_geometry_count; ++i) {
//         if(NULL != geometry_registry_->lit_mesh_geometries[i]) {
//             tmp_name = lit_mesh_geometry_name_get(geometry_registry_->lit_mesh_geometries[i]);
//             if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
//                 tmp_slot = i;
//                 found = true;
//                 break;
//             }
//         }
//     }

//     if(found) {
//         *out_index_ = tmp_slot;
//     }
//     return found;
// }

// // 本関数を呼び出す前に以下の条件はチェックしておくこと
// // name_ != NULL
// // geometry_registry_ != NULL
// // geometry_registry_internal_state_check(geometry_registry_) == true
// // out_index_ != NULL
// static bool line_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_) {
//     const char* tmp_name = NULL;
//     size_t tmp_slot = 0;
//     bool found = false;

//     if(0 == geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count) {
//         return false;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_line_mesh_geometry_count; ++i) {
//         if(NULL != geometry_registry_->line_mesh_geometries[i]) {
//             tmp_name = line_mesh_geometry_name_get(geometry_registry_->line_mesh_geometries[i]);
//             if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
//                 tmp_slot = i;
//                 found = true;
//                 break;
//             }
//         }
//     }

//     if(found) {
//         *out_index_ = tmp_slot;
//     }
//     return found;
// }

// // 本関数を呼び出す前に以下の条件はチェックしておくこと
// // name_ != NULL
// // geometry_registry_ != NULL
// // geometry_registry_internal_state_check(geometry_registry_) == true
// // out_index_ != NULL
// static bool point_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_) {
//     const char* tmp_name = NULL;
//     size_t tmp_slot = 0;
//     bool found = false;

//     if(0 == geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count) {
//         return false;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_point_mesh_geometry_count; ++i) {
//         if(NULL != geometry_registry_->point_mesh_geometries[i]) {
//             tmp_name = point_mesh_geometry_name_get(geometry_registry_->point_mesh_geometries[i]);
//             if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
//                 tmp_slot = i;
//                 found = true;
//                 break;
//             }
//         }
//     }

//     if(found) {
//         *out_index_ = tmp_slot;
//     }
//     return found;
// }

// // 本関数を呼び出す前に以下の条件はチェックしておくこと
// // name_ != NULL
// // geometry_registry_ != NULL
// // geometry_registry_internal_state_check(geometry_registry_) == true
// // out_index_ != NULL
// static bool ui_mesh_geometry_find(const char* name_, const geometry_registry_t* geometry_registry_, size_t* out_index_) {
//     const char* tmp_name = NULL;
//     size_t tmp_slot = 0;
//     bool found = false;

//     if(0 == geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count) {
//         return false;
//     }
//     for(size_t i = 0; i != geometry_registry_->geometry_registry_config.max_ui_mesh_geometry_count; ++i) {
//         if(NULL != geometry_registry_->ui_mesh_geometries[i]) {
//             tmp_name = ui_mesh_geometry_name_get(geometry_registry_->ui_mesh_geometries[i]);
//             if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
//                 tmp_slot = i;
//                 found = true;
//                 break;
//             }
//         }
//     }

//     if(found) {
//         *out_index_ = tmp_slot;
//     }
//     return found;
// }
