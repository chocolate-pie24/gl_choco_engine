#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // for sscanf

#include "engine/resource/loaders/stl_loader.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/resource_core/resource_err_utils.h"

// #define TEST_BUILD

/**
 * @brief STLファイルローダー内部情報管理構造体
 *
 */
struct stl_loader {
    point_normal_vertex_t* vertices;
    size_t vertex_count;
};

static resource_result_t vertex_count_calc(const char* path_, const char* name_, const char* extension_, size_t* out_vertex_count_);

resource_result_t stl_loader_create(stl_loader_t** stl_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    stl_loader_t* tmp_stl_loader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_create", "stl_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_create", "*stl_loader_")

    ret_mem = memory_system_allocate(sizeof(stl_loader_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_stl_loader);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("stl_loader_create(%s) - Failed to allocate memory for tmp_stl_loader.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_stl_loader->vertex_count = 0;
    tmp_stl_loader->vertices = NULL;
    *stl_loader_ = tmp_stl_loader;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_SUCCESS != ret) {
        if(NULL != tmp_stl_loader) {
            memory_system_free(tmp_stl_loader, sizeof(stl_loader_t), MEMORY_TAG_GEOMETRY);
            tmp_stl_loader = NULL;
        }
    }
    return ret;
}

void stl_loader_destroy(stl_loader_t** stl_loader_) {
    if(NULL == stl_loader_) {
        return;
    }
    if(NULL == *stl_loader_) {
        return;
    }
    if(NULL != (*stl_loader_)->vertices && 0 != (*stl_loader_)->vertex_count) {
        memory_system_free((*stl_loader_)->vertices, sizeof(point_normal_vertex_t) * (*stl_loader_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*stl_loader_)->vertices = NULL;
        (*stl_loader_)->vertex_count = 0;
    }

    memory_system_free(*stl_loader_, sizeof(stl_loader_t), MEMORY_TAG_GEOMETRY);
    *stl_loader_ = NULL;
}

resource_result_t stl_loader_load(const char* path_, const char* name_, const char* extension_, stl_loader_t* stl_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* string = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    size_t vertex_count = 0;
    size_t vertex_index = 0;
    size_t facet_vertex_count = 0;
    bool complete = false;
    bool normal_loaded = false;
    int parse_result = 0;
    vec3f_t tmp_normal = { 0 };
    vec3f_t tmp_vertex = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "stl_loader_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_load", "stl_loader_->vertex_count")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_load", "stl_loader_->vertices")

    ret = vertex_count_calc(path_, name_, extension_, &vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to count vertex.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to allocate memory for tmp_vertices.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to create fs_utils.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to create string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_utils = fs_utils_text_file_line_read(fs_utils, string);
        if(FS_UTILS_EOF == ret_fs_utils) {
            complete = true;
        } else if(FS_UTILS_SUCCESS == ret_fs_utils) {
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                parse_result = sscanf(choco_string_c_str(string), "  facet normal %f %f %f", &tmp_normal.elem[0], &tmp_normal.elem[1], &tmp_normal.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Failed to parse normal line. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                }
                if(-1.0f > tmp_normal.elem[0] || -1.0f > tmp_normal.elem[1] || -1.0f > tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Failed to parse normal line. Invalid normal value. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                } else if(1.0f < tmp_normal.elem[0] || 1.0f < tmp_normal.elem[1] || 1.0f < tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Failed to parse normal line. Invalid normal value. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                }
                normal_loaded = true;
                facet_vertex_count = 0;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if(!normal_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Failed to parse vertex line. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                }
                parse_result = sscanf(choco_string_c_str(string), "      vertex %f %f %f", &tmp_vertex.elem[0], &tmp_vertex.elem[1], &tmp_vertex.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Failed to parse vertex line. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                }
                if(vertex_index > vertex_count) {
                    ret = RESOURCE_UNDEFINED_ERROR;
                    ERROR_MESSAGE("stl_loader_load(%s) - Undefined error.", resource_rslt_to_str(ret));
                    goto cleanup;
                }

                tmp_vertices[vertex_index].normal.elem[0] = (int8_t)(127.5f * tmp_normal.elem[0]);
                tmp_vertices[vertex_index].normal.elem[1] = (int8_t)(127.5f * tmp_normal.elem[1]);
                tmp_vertices[vertex_index].normal.elem[2] = (int8_t)(127.5f * tmp_normal.elem[2]);

                tmp_vertices[vertex_index].position.elem[0] = tmp_vertex.elem[0];
                tmp_vertices[vertex_index].position.elem[1] = tmp_vertex.elem[1];
                tmp_vertices[vertex_index].position.elem[2] = tmp_vertex.elem[2];
                vertex_index++;
                facet_vertex_count++;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "endfacet")) {
                normal_loaded = false;
                if(3 != facet_vertex_count) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_load(%s) - Invalid facet format. line = '%s'.", resource_rslt_to_str(ret), choco_string_c_str(string));
                    goto cleanup;
                }
            }
        } else {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("stl_loader_load(%s) - Failed to read line from stl file.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if(vertex_count != vertex_index) {
        ret = RESOURCE_UNDEFINED_ERROR;
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to load stl file.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    stl_loader_->vertex_count = vertex_count;
    stl_loader_->vertices = tmp_vertices;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != fs_utils) {
        fs_utils_destroy(&fs_utils);
    }
    if(NULL != string) {
        choco_string_destroy(&string);
    }
    if(RESOURCE_SUCCESS != ret && NULL != tmp_vertices) {
        memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        tmp_vertices = NULL;
    }
    return ret;
}

resource_result_t stl_loader_vertex_move(stl_loader_t* stl_loader_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_move", "stl_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_move", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_move", "*out_vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertex_move", "stl_loader_->vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertex_move", "stl_loader_->vertices")

    *out_vertices_ = stl_loader_->vertices;
    *out_vertex_count_ = stl_loader_->vertex_count;

    stl_loader_->vertices = NULL;
    stl_loader_->vertex_count = 0;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t stl_loader_vertex_count_get(const stl_loader_t* stl_loader_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_get", "stl_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_get", "out_vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertex_count_get", "stl_loader_->vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertex_count_get", "stl_loader_->vertices")

    *out_vertex_count_ = stl_loader_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t vertex_count_calc(const char* path_, const char* name_, const char* extension_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* string = NULL;

    size_t normal_count = 0;
    size_t vertex_count = 0;

    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_calc", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_calc", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_calc", "extension_")

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("stl_loader_load(%s) - Failed to create fs_utils.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("vertex_count_calc(%s) - Failed to create string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_utils = fs_utils_text_file_line_read(fs_utils, string);
        if(FS_UTILS_EOF == ret_fs_utils) {
            complete = true;
        } else if(FS_UTILS_SUCCESS == ret_fs_utils) {
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if((SIZE_MAX - 1) < normal_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_calc(%s) - Provided stl file is too long.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                normal_count++;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if((SIZE_MAX - 1) < vertex_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_calc(%s) - Provided stl file is too long.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                vertex_count++;
            }
        } else {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("vertex_count_calc(%s) - Failed to read line from stl file.", resource_rslt_to_str(ret));
            goto cleanup;
        }
    }

    if((normal_count * 3) != vertex_count) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("vertex_count_calc(%s) - Provided stl file is corrupted.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_vertex_count_ = vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != string) {
        choco_string_destroy(&string);
    }
    if(NULL != fs_utils) {
        fs_utils_destroy(&fs_utils);
    }
    return ret;
}
