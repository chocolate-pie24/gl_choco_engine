// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // for sscanf

#include "engine/resource/loaders/stl_loader.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/memory/choco_memory.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// NOTE:
// - TODO: binary_loadの追加
// - TODO: format_detectの追加(ascii, binary判定)
// - custom_formatの出力、loadは別モジュールにする

static resource_result_t ascii_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_);
static resource_result_t vertex_count_calc(const char* fullpath_, size_t* out_vertex_count_);

resource_result_t stl_loader_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    size_t tmp_vertex_count = 0;
    point_normal_vertex_t* tmp_vertices = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_load", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_load", "*out_vertices_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("stl_loader_load(%s) - Provided fullpath_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = ascii_load(fullpath_, &tmp_vertex_count, &tmp_vertices);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("stl_loader_load(%s) - ascii_load failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_vertex_count_ = tmp_vertex_count;
    *out_vertices_ = tmp_vertices;
    tmp_vertices = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t ascii_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    choco_string_t* string = NULL;
    point_normal_vertex_t* tmp_vertices = NULL;

    size_t line_count = 0;
    size_t vertex_count = 0;
    size_t vertex_index = 0;
    size_t facet_vertex_count = 0;

    bool complete = false;

    bool facet_normal_loaded = false;
    bool outer_loop_loaded = false;
    bool facet_vertices_loaded = false;
    bool endloop_loaded = false;

    int parse_result = 0;
    vec3f_t tmp_normal = { 0 };
    vec3f_t tmp_vertex = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ascii_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "ascii_load", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ascii_load", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "ascii_load", "*out_vertices_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("ascii_load(%s) - Provided fullpath_ is not valid.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret = vertex_count_calc(fullpath_, &vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("ascii_load(%s) - Failed to calculate vertex count for ASCII STL file.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("ascii_load(%s) - Vertex buffer size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(point_normal_vertex_t));
        goto cleanup;
    }
    ret_memory_system = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_memory_system) {
        ret = resource_rslt_convert_choco_memory(ret_memory_system);
        ERROR_MESSAGE("ascii_load(%s) - Failed to allocate vertex buffer. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(point_normal_vertex_t));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("ascii_load(%s) - fs_stream_create failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ascii_load(%s) - Failed to create line buffer string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream, string);
        if(FS_STREAM_EOF == ret_fs_stream) {
            complete = true;
        } else if(FS_STREAM_SUCCESS == ret_fs_stream) {
            if((SIZE_MAX - 1) < line_count) {
                ret = RESOURCE_OVERFLOW;
                ERROR_MESSAGE("ascii_load(%s) - ASCII STL line count overflowed size_t while loading.", resource_rslt_to_str(ret));
                goto cleanup;
            }
            line_count++;
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if(facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {  // データ不整合チェック
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Unexpected 'facet normal' at line %zu. Previous facet is incomplete. Expected 'endfacet' before starting a new facet.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                parse_result = sscanf(choco_string_c_str(string), "  facet normal %f %f %f", &tmp_normal.elem[0], &tmp_normal.elem[1], &tmp_normal.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Failed to parse 'facet normal' at line %zu. Expected 3 float values. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                if(!vec3f_is_finite(tmp_normal)) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Invalid normal value at line %zu. Normal contains NaN or infinity. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                } else if(-1.0f > tmp_normal.elem[0] || -1.0f > tmp_normal.elem[1] || -1.0f > tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Invalid normal value at line %zu. Each normal component must be in [-1.0, 1.0]. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                } else if(1.0f < tmp_normal.elem[0] || 1.0f < tmp_normal.elem[1] || 1.0f < tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Invalid normal value at line %zu. Each normal component must be in [-1.0, 1.0]. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                facet_normal_loaded = true;
                facet_vertex_count = 0;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "outer loop")) {
                if(!facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Unexpected 'outer loop' at line %zu. Expected a valid 'facet normal' before 'outer loop'.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                outer_loop_loaded = true;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if(!facet_normal_loaded || !outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Unexpected 'vertex' at line %zu. Expected 'facet normal' and 'outer loop' before vertex lines, and expected no completed vertex list before 'endloop'.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                parse_result = sscanf(choco_string_c_str(string), "      vertex %f %f %f", &tmp_vertex.elem[0], &tmp_vertex.elem[1], &tmp_vertex.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Failed to parse 'vertex' at line %zu. Expected 3 float values. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                if(vertex_index >= vertex_count) {  // 頂点数読み込みと現在との間にファイル状態変化
                    ret = RESOURCE_RUNTIME_ERROR;
                    ERROR_MESSAGE("ascii_load(%s) - Vertex index exceeded precomputed vertex count at line %zu. The STL file may have changed during loading, or the count pass and parse pass are inconsistent. vertex_index = %zu, vertex_count = %zu.", resource_rslt_to_str(ret), line_count, vertex_index, vertex_count);
                    goto cleanup;
                }
                if(!vec3f_is_finite(tmp_vertex)) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Invalid vertex value at line %zu. Vertex contains NaN or infinity. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }

                tmp_vertices[vertex_index].normal.elem[0] = (int8_t)(127.5f * tmp_normal.elem[0]);
                tmp_vertices[vertex_index].normal.elem[1] = (int8_t)(127.5f * tmp_normal.elem[1]);
                tmp_vertices[vertex_index].normal.elem[2] = (int8_t)(127.5f * tmp_normal.elem[2]);

                tmp_vertices[vertex_index].position = tmp_vertex;

                vertex_index++;
                facet_vertex_count++;
                if(3 == facet_vertex_count) {
                    facet_vertices_loaded = true;
                }
            } else if(choco_string_substring_exists(choco_string_c_str(string), "endloop")) {
                if(!facet_normal_loaded || !outer_loop_loaded || !facet_vertices_loaded || endloop_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Unexpected 'endloop' at line %zu. Expected exactly 3 vertex lines before 'endloop'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
                    goto cleanup;
                }
                endloop_loaded = true;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "endfacet")) {
                if(!facet_normal_loaded || !outer_loop_loaded || !facet_vertices_loaded || !endloop_loaded || 3 != facet_vertex_count) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("ascii_load(%s) - Unexpected 'endfacet' at line %zu. Expected 'facet normal', 'outer loop', 3 vertices, and 'endloop' before 'endfacet'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
                    goto cleanup;
                }
                facet_normal_loaded = false;
                outer_loop_loaded = false;
                facet_vertices_loaded = false;
                endloop_loaded = false;
                facet_vertex_count = 0;
            }
        } else {
            ret = resource_rslt_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("ascii_load(%s) - Failed to read ASCII STL line. expected_line = %zu.", resource_rslt_to_str(ret), line_count + 1);
            goto cleanup;
        }
    }

    if(facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded || 0 != facet_vertex_count) {    // endfacetがないままデータ終了
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("ascii_load(%s) - Unexpected EOF while parsing ASCII STL facet. Last successfully read line = %zu. The current facet was not closed by 'endfacet'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
        goto cleanup;
    }
    if(vertex_count != vertex_index) {
        ret = RESOURCE_RUNTIME_ERROR;
        ERROR_MESSAGE("ascii_load(%s) - Loaded vertex count does not match precomputed vertex count. loaded = %zu, expected = %zu.", resource_rslt_to_str(ret), vertex_index, vertex_count);
        goto cleanup;
    }

    *out_vertex_count_ = vertex_count;
    *out_vertices_ = tmp_vertices;
    tmp_vertices = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != fs_stream) {
        fs_stream_destroy(&fs_stream, NULL);
    }
    if(NULL != string) {
        choco_string_destroy(&string);
    }
    if(NULL != tmp_vertices) {
        memory_system_free(tmp_vertices, sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        tmp_vertices = NULL;
    }
    return ret;
}

static resource_result_t vertex_count_calc(const char* fullpath_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    choco_string_t* string = NULL;

    size_t line_count = 0;
    size_t normal_count = 0;
    size_t vertex_count = 0;

    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_calc", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_calc", "out_vertex_count_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("vertex_count_calc(%s) - Provided fullpath_ is not valid.", resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT));
        goto cleanup;
    }

    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_rslt_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("vertex_count_calc(%s) - fs_stream_create failed.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("vertex_count_calc(%s) - Failed to create line buffer string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream, string);
        if(FS_STREAM_EOF == ret_fs_stream) {
            complete = true;
        } else if(FS_STREAM_SUCCESS == ret_fs_stream) {
            if((SIZE_MAX - 1) < line_count) {
                ret = RESOURCE_OVERFLOW;
                ERROR_MESSAGE("vertex_count_calc(%s) - ASCII STL line count overflowed size_t while loading.", resource_rslt_to_str(ret));
                goto cleanup;
            }
            line_count++;
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if((SIZE_MAX - 1) < normal_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                normal_count++;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if((SIZE_MAX - 1) < vertex_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                vertex_count++;
            }
        } else {
            ret = resource_rslt_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("vertex_count_calc(%s) - Failed to read ASCII STL line while counting vertices. expected_line = %zu.", resource_rslt_to_str(ret), line_count + 1);
            goto cleanup;
        }
    }

    if((SIZE_MAX / 3) < normal_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == vertex_count || (normal_count * 3) != vertex_count) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("vertex_count_calc(%s) - Invalid ASCII STL structure. vertex_count must be 3 times normal_count and greater than 0. normal_count = %zu, vertex_count = %zu.", resource_rslt_to_str(ret), normal_count, vertex_count);
        goto cleanup;
    }

    *out_vertex_count_ = vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(NULL != string) {
        choco_string_destroy(&string);
    }
    if(NULL != fs_stream) {
        fs_stream_destroy(&fs_stream, NULL);
    }
    return ret;
}
