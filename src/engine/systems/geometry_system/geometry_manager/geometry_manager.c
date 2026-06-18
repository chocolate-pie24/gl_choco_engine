#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/systems/geometry_system/geometry_manager/geometry_manager.h"

#include "engine/systems/geometry_system/geometry_system_core/geometry_system_types.h"
#include "engine/systems/geometry_system/geometry_system_core/geometry_system_err_utils.h"

#include "engine/resource/geometry/line_mesh_geometry.h"
#include "engine/resource/geometry/lit_mesh_geometry.h"
#include "engine/resource/geometry/point_mesh_geometry.h"
#include "engine/resource/geometry/ui_mesh_geometry.h"

#include "engine/containers/choco_string.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

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

static bool geometry_type_valid_check(geometry_type_t geometry_type_);
static bool geometry_system_internal_state_check(geometry_system_t* geometry_system_);
static bool lit_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_);
static bool line_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_);
static bool point_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_);
static bool ui_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_);

geometry_system_result_t geometry_system_initialize(const geometry_system_config_t* config_, linear_alloc_t* allocator_, geometry_system_t** out_geometry_system_) {
    geometry_system_result_t ret = GEOMETRY_SYSTEM_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    geometry_system_t* tmp_system = NULL;
    lit_mesh_geometry_t** tmp_lit_mesh_geometries = NULL;
    line_mesh_geometry_t** tmp_line_mesh_geometries = NULL;
    point_mesh_geometry_t** tmp_point_mesh_geometries = NULL;
    ui_mesh_geometry_t** tmp_ui_mesh_geometries = NULL;

    size_t* tmp_lit_mesh_geometry_vertex_offset = NULL;
    size_t* tmp_line_mesh_geometry_vertex_offset = NULL;
    size_t* tmp_point_mesh_geometry_vertex_offset = NULL;
    size_t* tmp_ui_mesh_geometry_vertex_offset = NULL;

    // config_内の各Geometry最大数は0を許容する
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_initialize", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_system_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_initialize", "out_geometry_system_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_geometry_system_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_initialize", "*out_geometry_system_")

    // geometry_system_tメモリ確保
    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(geometry_system_t), alignof(geometry_system_t), (void**)&tmp_system);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for geometry_system_t instance.", geometry_system_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_system, 0, sizeof(geometry_system_t));

    if(0 != config_->max_line_mesh_geometry_count) {
        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(line_mesh_geometry_t*) * config_->max_line_mesh_geometry_count, alignof(line_mesh_geometry_t*), (void**)&tmp_line_mesh_geometries);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for line_mesh_geometry_t* array instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_line_mesh_geometry_count, alignof(size_t*), (void**)&tmp_line_mesh_geometry_vertex_offset);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for vertex offset array(line mesh) instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(0 != config_->max_lit_mesh_geometry_count) {
        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(lit_mesh_geometry_t*) * config_->max_lit_mesh_geometry_count, alignof(lit_mesh_geometry_t*), (void**)&tmp_lit_mesh_geometries);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for lit_mesh_geometry_t* array instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_lit_mesh_geometry_count, alignof(size_t*), (void**)&tmp_lit_mesh_geometry_vertex_offset);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for vertex offset array(lit mesh) instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(0 != config_->max_point_mesh_geometry_count) {
        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(point_mesh_geometry_t*) * config_->max_point_mesh_geometry_count, alignof(point_mesh_geometry_t*), (void**)&tmp_point_mesh_geometries);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for point_mesh_geometry_t* array instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_point_mesh_geometry_count, alignof(size_t*), (void**)&tmp_point_mesh_geometry_vertex_offset);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for vertex offset array(point mesh) instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(0 != config_->max_ui_mesh_geometry_count) {
        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(ui_mesh_geometry_t*) * config_->max_ui_mesh_geometry_count, alignof(ui_mesh_geometry_t*), (void**)&tmp_ui_mesh_geometries);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for ui_mesh_geometry_t* array instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }

        ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(size_t*) * config_->max_ui_mesh_geometry_count, alignof(size_t*), (void**)&tmp_ui_mesh_geometry_vertex_offset);
        if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
            ret = geometry_system_rslt_convert_linear_alloc(ret_linear_alloc);
            ERROR_MESSAGE("geometry_system_initialize(%s) - Failed to allocate memory for vertex offset array(ui mesh) instance.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        }
    }

    tmp_system->geometry_system_config = *config_;

    tmp_system->line_mesh_geometries = tmp_line_mesh_geometries;
    tmp_system->lit_mesh_geometries = tmp_lit_mesh_geometries;
    tmp_system->point_mesh_geometries = tmp_point_mesh_geometries;
    tmp_system->ui_mesh_geometries = tmp_ui_mesh_geometries;

    tmp_system->line_mesh_geometry_vertex_offset = tmp_line_mesh_geometry_vertex_offset;
    tmp_system->lit_mesh_geometry_vertex_offset = tmp_lit_mesh_geometry_vertex_offset;
    tmp_system->point_mesh_geometry_vertex_offset = tmp_point_mesh_geometry_vertex_offset;
    tmp_system->ui_mesh_geometry_vertex_offset = tmp_ui_mesh_geometry_vertex_offset;

    *out_geometry_system_ = tmp_system;

    ret = GEOMETRY_SYSTEM_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

void geometry_system_deinitialize(geometry_system_t* geometry_system_) {
    if(NULL == geometry_system_) {
        return;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_line_mesh_geometry_count; ++i) {
        line_mesh_geometry_destroy(&geometry_system_->line_mesh_geometries[i]);
        geometry_system_->line_mesh_geometry_vertex_offset[i] = 0;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_lit_mesh_geometry_count; ++i) {
        lit_mesh_geometry_destroy(&geometry_system_->lit_mesh_geometries[i]);
        geometry_system_->lit_mesh_geometry_vertex_offset[i] = 0;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_point_mesh_geometry_count; ++i) {
        point_mesh_geometry_destroy(&geometry_system_->point_mesh_geometries[i]);
        geometry_system_->point_mesh_geometry_vertex_offset[i] = 0;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_ui_mesh_geometry_count; ++i) {
        ui_mesh_geometry_destroy(&geometry_system_->ui_mesh_geometries[i]);
        geometry_system_->ui_mesh_geometry_vertex_offset[i] = 0;
    }
}

// 失敗時out引数不変
// TODO: application->renderer_frontend->renderer_backendまでの流れを考える
// TODO: scene, model, mesh(point, stl, obj)を元にgeometry_systemの役割を整理
// TODO: gpuアップロードのタイミングを考える
// TODO: このAPIは必要か？運用を考える
// TODO: テクスチャはGPUアップロード後にピクセルをリリースする、一方でジオメトリはリリースしない。これの整合性を取る
geometry_system_result_t geometry_system_draw_range_get_by_name(geometry_type_t geometry_type_, const char* name_, const geometry_system_t* geometry_system_, size_t* out_vertex_offset_, size_t* out_vertex_count_) {
    geometry_system_result_t ret = GEOMETRY_SYSTEM_INVALID_ARGUMENT;
    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    const char* tmp_name = NULL;
    size_t tmp_vertex_offset = 0;
    size_t tmp_vertex_count = 0;
    size_t slot = 0;
    bool found = false;

    IF_ARG_FALSE_GOTO_CLEANUP(geometry_type_valid_check(geometry_type_), ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "geometry_type_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP('\0' != name_[0], ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "name_[0]")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_system_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "geometry_system_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_offset_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "out_vertex_offset_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, GEOMETRY_SYSTEM_INVALID_ARGUMENT, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_INVALID_ARGUMENT), "geometry_system_draw_range_get_by_name", "out_vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(geometry_system_internal_state_check(geometry_system_), ret, GEOMETRY_SYSTEM_DATA_CORRUPTED, geometry_system_rslt_to_str(GEOMETRY_SYSTEM_DATA_CORRUPTED), "geometry_system_draw_range_get_by_name", "geometry_system_")

    if(GEOMETRY_TYPE_LIT_MESH_GEOMETRY == geometry_type_) {
        if(!lit_mesh_geometry_find(name_, geometry_system_, &slot)) {
            // TODO: ここでロードするかを考える
            ret = GEOMETRY_SYSTEM_BAD_OPERATION;
            ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Provided geometry name not found.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        } else {
            ret_resource = lit_mesh_geometry_vertex_count_get(geometry_system_->lit_mesh_geometries[slot], &tmp_vertex_count);
            if(RESOURCE_SUCCESS != ret_resource) {
                ret = geometry_system_rslt_convert_resource(ret_resource);
                ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Failed to get vertex count from geometry. geometry name = '%s'.", geometry_system_rslt_to_str(ret), name_);
                goto cleanup;
            }
            tmp_vertex_offset = geometry_system_->lit_mesh_geometry_vertex_offset[slot];
        }
    } else if(GEOMETRY_TYPE_LINE_MESH_GEOMETRY == geometry_type_) {
        if(!line_mesh_geometry_find(name_, geometry_system_, &slot)) {
            ret = GEOMETRY_SYSTEM_BAD_OPERATION;
            ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Provided geometry name not found.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        } else {
            ret_resource = line_mesh_geometry_vertex_count_get(geometry_system_->line_mesh_geometries[slot], &tmp_vertex_count);
            if(RESOURCE_SUCCESS != ret_resource) {
                ret = geometry_system_rslt_convert_resource(ret_resource);
                ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Failed to get vertex count from geometry. geometry name = '%s'.", geometry_system_rslt_to_str(ret), name_);
                goto cleanup;
            }
            tmp_vertex_offset = geometry_system_->line_mesh_geometry_vertex_offset[slot];
        }
    } else if(GEOMETRY_TYPE_POINT_MESH_GEOMETRY == geometry_type_) {
        if(!point_mesh_geometry_find(name_, geometry_system_, &slot)) {
            ret = GEOMETRY_SYSTEM_BAD_OPERATION;
            ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Provided geometry name not found.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        } else {
            ret_resource = point_mesh_geometry_vertex_count_get(geometry_system_->point_mesh_geometries[slot], &tmp_vertex_count);
            if(RESOURCE_SUCCESS != ret_resource) {
                ret = geometry_system_rslt_convert_resource(ret_resource);
                ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Failed to get vertex count from geometry. geometry name = '%s'.", geometry_system_rslt_to_str(ret), name_);
                goto cleanup;
            }
            tmp_vertex_offset = geometry_system_->point_mesh_geometry_vertex_offset[slot];
        }
    } else if(GEOMETRY_TYPE_UI_MESH_GEOMETRY == geometry_type_) {
        if(!ui_mesh_geometry_find(name_, geometry_system_, &slot)) {
            ret = GEOMETRY_SYSTEM_BAD_OPERATION;
            ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Provided geometry name not found.", geometry_system_rslt_to_str(ret));
            goto cleanup;
        } else {
            ret_resource = ui_mesh_geometry_vertex_count_get(geometry_system_->ui_mesh_geometries[slot], &tmp_vertex_count);
            if(RESOURCE_SUCCESS != ret_resource) {
                ret = geometry_system_rslt_convert_resource(ret_resource);
                ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - Failed to get vertex count from geometry. geometry name = '%s'.", geometry_system_rslt_to_str(ret), name_);
                goto cleanup;
            }
            tmp_vertex_offset = geometry_system_->ui_mesh_geometry_vertex_offset[slot];
        }
    } else {
        ret = GEOMETRY_PRIMITIVE_RUNTIME_ERROR;
        ERROR_MESSAGE("geometry_system_draw_range_get_by_name(%s) - implementation error.", geometry_system_rslt_to_str(ret));
        goto cleanup;
    }

    *out_vertex_count_ = tmp_vertex_count;
    *out_vertex_offset_ = tmp_vertex_offset;

    ret = GEOMETRY_SYSTEM_SUCCESS;

cleanup:
    return ret;
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

static bool geometry_type_valid_check(geometry_type_t geometry_type_) {
    switch(geometry_type_) {
    case GEOMETRY_TYPE_LIT_MESH_GEOMETRY:
        return true;
    case GEOMETRY_TYPE_LINE_MESH_GEOMETRY:
        return true;
    case GEOMETRY_TYPE_POINT_MESH_GEOMETRY:
        return true;
    case GEOMETRY_TYPE_UI_MESH_GEOMETRY:
        return true;
    default:
        return false;
    }
}

static bool geometry_system_internal_state_check(geometry_system_t* geometry_system_) {
    if(0 != geometry_system_->geometry_system_config.max_line_mesh_geometry_count && (NULL == geometry_system_->line_mesh_geometries || NULL == geometry_system_->line_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 != geometry_system_->geometry_system_config.max_lit_mesh_geometry_count && (NULL == geometry_system_->lit_mesh_geometries || NULL == geometry_system_->lit_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 != geometry_system_->geometry_system_config.max_point_mesh_geometry_count && (NULL == geometry_system_->point_mesh_geometries || NULL == geometry_system_->point_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 != geometry_system_->geometry_system_config.max_ui_mesh_geometry_count && (NULL == geometry_system_->ui_mesh_geometries || NULL == geometry_system_->ui_mesh_geometry_vertex_offset)) {
        return false;
    }

    if(0 == geometry_system_->geometry_system_config.max_line_mesh_geometry_count && (NULL != geometry_system_->line_mesh_geometries || NULL != geometry_system_->line_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 == geometry_system_->geometry_system_config.max_lit_mesh_geometry_count && (NULL != geometry_system_->lit_mesh_geometries || NULL != geometry_system_->lit_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 == geometry_system_->geometry_system_config.max_point_mesh_geometry_count && (NULL != geometry_system_->point_mesh_geometries || NULL != geometry_system_->point_mesh_geometry_vertex_offset)) {
        return false;
    } else if(0 == geometry_system_->geometry_system_config.max_ui_mesh_geometry_count && (NULL != geometry_system_->ui_mesh_geometries || NULL != geometry_system_->ui_mesh_geometry_vertex_offset)) {
        return false;
    }

    return true;
}

// 本関数を呼び出す前に以下の条件はチェックしておくこと
// name_ != NULL
// geometry_system_ != NULL
// geometry_system_internal_state_check(geometry_system_) == true
// out_index_ != NULL
static bool lit_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_) {
    const char* tmp_name = NULL;
    size_t tmp_slot = 0;
    bool found = false;

    if(0 == geometry_system_->geometry_system_config.max_lit_mesh_geometry_count) {
        return false;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_lit_mesh_geometry_count; ++i) {
        if(NULL != geometry_system_->lit_mesh_geometries[i]) {
            tmp_name = lit_mesh_geometry_name_get(geometry_system_->lit_mesh_geometries[i]);
            if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
                tmp_slot = i;
                found = true;
                break;
            }
        }
    }

    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}

// 本関数を呼び出す前に以下の条件はチェックしておくこと
// name_ != NULL
// geometry_system_ != NULL
// geometry_system_internal_state_check(geometry_system_) == true
// out_index_ != NULL
static bool line_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_) {
    const char* tmp_name = NULL;
    size_t tmp_slot = 0;
    bool found = false;

    if(0 == geometry_system_->geometry_system_config.max_line_mesh_geometry_count) {
        return false;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_line_mesh_geometry_count; ++i) {
        if(NULL != geometry_system_->line_mesh_geometries[i]) {
            tmp_name = line_mesh_geometry_name_get(geometry_system_->line_mesh_geometries[i]);
            if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
                tmp_slot = i;
                found = true;
                break;
            }
        }
    }

    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}

// 本関数を呼び出す前に以下の条件はチェックしておくこと
// name_ != NULL
// geometry_system_ != NULL
// geometry_system_internal_state_check(geometry_system_) == true
// out_index_ != NULL
static bool point_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_) {
    const char* tmp_name = NULL;
    size_t tmp_slot = 0;
    bool found = false;

    if(0 == geometry_system_->geometry_system_config.max_point_mesh_geometry_count) {
        return false;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_point_mesh_geometry_count; ++i) {
        if(NULL != geometry_system_->point_mesh_geometries[i]) {
            tmp_name = point_mesh_geometry_name_get(geometry_system_->point_mesh_geometries[i]);
            if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
                tmp_slot = i;
                found = true;
                break;
            }
        }
    }

    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}

// 本関数を呼び出す前に以下の条件はチェックしておくこと
// name_ != NULL
// geometry_system_ != NULL
// geometry_system_internal_state_check(geometry_system_) == true
// out_index_ != NULL
static bool ui_mesh_geometry_find(const char* name_, geometry_system_t* geometry_system_, size_t* out_index_) {
    const char* tmp_name = NULL;
    size_t tmp_slot = 0;
    bool found = false;

    if(0 == geometry_system_->geometry_system_config.max_ui_mesh_geometry_count) {
        return false;
    }
    for(size_t i = 0; i != geometry_system_->geometry_system_config.max_ui_mesh_geometry_count; ++i) {
        if(NULL != geometry_system_->ui_mesh_geometries[i]) {
            tmp_name = ui_mesh_geometry_name_get(geometry_system_->ui_mesh_geometries[i]);
            if(NULL != tmp_name && choco_string_equal(tmp_name, name_)) {
                tmp_slot = i;
                found = true;
                break;
            }
        }
    }

    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}
