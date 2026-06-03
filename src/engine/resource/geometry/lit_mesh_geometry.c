#include "engine/resource/geometry/lit_mesh_geometry.h"

#include <stddef.h>

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

#include "engine/resource/loaders/stl_loader.h"

#include "engine/containers/choco_string.h"

#include "engine/core/geometry_primitive/vertex.h"
#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"
#include "engine/base/choco_math/math_types.h"

struct lit_mesh_geometry {
    choco_string_t* name;

    size_t vertex_count;
    point_normal_vertex_t* vertices;
};

resource_result_t lit_mesh_geometry_create(lit_mesh_geometry_t** geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    lit_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_create", "*geometry_")

    ret_mem = memory_system_allocate(sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_geometry);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_create(%s) - Failed to allocate lit_mesh_geometry_t instance.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_geometry->name = NULL;
    tmp_geometry->vertex_count = 0;
    tmp_geometry->vertices = NULL;

    *geometry_ = tmp_geometry;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_geometry) {
            memory_system_free(tmp_geometry, sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
            tmp_geometry = NULL;
        }
    }
    return ret;
}

void lit_mesh_geometry_destroy(lit_mesh_geometry_t** geometry_) {
    if(NULL == geometry_) {
        return;
    }
    if(NULL == *geometry_) {
        return;
    }
    if(NULL != (*geometry_)->name) {
        choco_string_destroy(&(*geometry_)->name);
    }
    if(NULL != (*geometry_)->vertices && 0 != (*geometry_)->vertex_count) {
        memory_system_free((*geometry_)->vertices, sizeof(point_normal_vertex_t) * (*geometry_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*geometry_)->vertices = NULL;
        (*geometry_)->vertex_count = 0;
    } else if(NULL != (*geometry_)->vertices && 0 == (*geometry_)->vertex_count) {
        ERROR_MESSAGE("lit_mesh_geometry_destroy(%s) - lit_mesh_geometry internal state is inconsistent: vertices is not NULL but vertex_count is 0. CPU-side vertex array was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    }

    memory_system_free(*geometry_, sizeof(lit_mesh_geometry_t), MEMORY_TAG_GEOMETRY);
    *geometry_ = NULL;
}

// lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止
// 失敗時はgeometry_は不変
resource_result_t lit_mesh_geometry_initialize_from_vertices(const char* name_, size_t vertex_count_, const point_normal_vertex_t* vertices_, lit_mesh_geometry_t* geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    choco_string_t* tmp_name = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "name_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_vertices", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_vertices", "geometry_->vertex_count")

    ret_string = choco_string_create_from_c_string(name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - Failed to create geometry name string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count_) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - CPU-side vertex array size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_vertices(%s) - Failed to allocate CPU-side vertex array. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count_, sizeof(point_normal_vertex_t));
        goto cleanup;
    }

    for(size_t i = 0; i != vertex_count_; ++i) {
        vec4i8_initialize(vertices_[i].normal.elem[0], vertices_[i].normal.elem[1], vertices_[i].normal.elem[2], vertices_[i].normal.elem[3], &tmp_vertices[i].normal);
        vec3f_initialize(vertices_[i].position.elem[0], vertices_[i].position.elem[1], vertices_[i].position.elem[2], &tmp_vertices[i].position);
    }

    geometry_->name = tmp_name;
    geometry_->vertex_count = vertex_count_;
    geometry_->vertices = tmp_vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
        if(NULL != tmp_vertices) {
            memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * vertex_count_, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    return ret;
}

// lit_mesh_geometry_tの内部リソースはlit_mesh_geometryが所有するため、一度初期化したあと、destroyをせずに再初期化するのは禁止
// 失敗時にはgeometry_は不変
resource_result_t lit_mesh_geometry_initialize_from_file(const char* path_, const char* name_, const char* extension_, lit_mesh_geometry_t* geometry_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    stl_loader_t* stl_loader = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;
    choco_string_t* tmp_name = NULL;
    size_t tmp_vertex_count = 0;

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_initialize_from_file", "geometry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->name")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_initialize_from_file", "geometry_->vertex_count")

    if(choco_string_equal(".stl", extension_)) {
        ret = stl_loader_create(&stl_loader);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to create stl_loader_t instance for lit mesh geometry loading.", resource_rslt_to_str(ret));
            goto cleanup;
        }
        ret = stl_loader_ascii_load(path_, name_, extension_, stl_loader);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to load ASCII STL file for lit mesh geometry. name = '%s', extension = '%s'.", resource_rslt_to_str(ret), name_, extension_);
            goto cleanup;
        }
        ret = stl_loader_vertices_move(stl_loader, &tmp_vertices, &tmp_vertex_count);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to move vertices from STL loader.", resource_rslt_to_str(ret));
            goto cleanup;
        }
        ret_string = choco_string_create_from_c_string(name_, &tmp_name);
        if(CHOCO_STRING_SUCCESS != ret_string) {
            ret = resource_rslt_convert_choco_string(ret_string);
            ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Failed to create lit mesh geometry name string.", resource_rslt_to_str(ret));
            goto cleanup;
        }

        geometry_->name = tmp_name;
        geometry_->vertex_count = tmp_vertex_count;
        geometry_->vertices = tmp_vertices;
    } else {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("lit_mesh_geometry_initialize_from_file(%s) - Unsupported lit mesh geometry file extension '%s'. Currently supported: '.stl' ASCII STL.", resource_rslt_to_str(ret), extension_);
        goto cleanup;
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
        if(NULL != tmp_vertices) {
            memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * tmp_vertex_count, MEMORY_TAG_GEOMETRY);
            tmp_vertices = NULL;
        }
    }
    if(NULL != stl_loader) {
        stl_loader_destroy(&stl_loader);
    }
    return ret;
}

const char* lit_mesh_geometry_name_get(const lit_mesh_geometry_t* geometry_) {
    if(NULL == geometry_) {
        return NULL;
    }
    if(NULL == geometry_->name) {
        return NULL;
    }
    return choco_string_c_str(geometry_->name);
}

resource_result_t lit_mesh_geometry_vertices_get(const lit_mesh_geometry_t* geometry_, const point_normal_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertices_get", "*out_vertices_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertices_get", "geometry_->vertex_count")

    *out_vertices_ = geometry_->vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t lit_mesh_geometry_vertex_count_get(const lit_mesh_geometry_t* geometry_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(geometry_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertex_count_get", "geometry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "lit_mesh_geometry_vertex_count_get", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->name, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->name")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->vertices")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != geometry_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "lit_mesh_geometry_vertex_count_get", "geometry_->vertex_count")

    *out_vertex_count_ = geometry_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}
