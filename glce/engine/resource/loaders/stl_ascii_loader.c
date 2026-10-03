// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/resource/loaders/stl_ascii_loader.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // for sscanf

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/io_utils/fs_stream.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// NOTE:
// - TODO: binary_loadの追加(別モジュール)
// - TODO: stl_loaderモジュールを追加し、format_detectの追加(ascii, binary判定+stl_ascii_loader, stl_binary_loader)

// ============================================================
// Private Type Definitions
// ============================================================
typedef struct triangle_data_block {
    choco_string_t* facet_normal_line;
    choco_string_t* outer_loop_line;
    choco_string_t* vertex_line1;
    choco_string_t* vertex_line2;
    choco_string_t* vertex_line3;
    choco_string_t* endloop_line;
    choco_string_t* endfacet_line;
} triangle_data_block_t;

typedef struct triangle_data {
    vec3f_t vertices[3];
    vec3f_t normal;
} triangle_data_t;

// ============================================================
// Private Function Declarations
// ============================================================
// Loading helpers
static resource_result_t vertex_count_preload(const char* fullpath_, size_t* out_triangle_count_, size_t* out_vertex_count_);
static resource_result_t triangle_data_block_load_next(fs_stream_t* fs_stream_, triangle_data_block_t* out_data_block_, bool* out_loaded_);

// Parsing helpers
static resource_result_t triangle_data_block_initialize(triangle_data_block_t* out_data_block_);
static void triangle_data_block_deinitialize(triangle_data_block_t* data_block_);
static resource_result_t triangle_data_block_parse(const triangle_data_block_t* data_block_, triangle_data_t* out_triangle_data_);

// Vertex normalization helpers
static resource_result_t triangle_normalize(const triangle_data_t* triangle_data_, point_normal_vertex_t out_vertex_[3]);

// Validators
static bool count_is_valid(size_t triangle_count_, size_t vertex_count_);
static bool triangle_data_block_is_valid(const triangle_data_block_t* data_block_);
static bool load_result_is_valid(const point_normal_vertex_t* vertices_, size_t vertex_count_);

// ============================================================
// Public API
// ============================================================
resource_result_t stl_ascii_loader_load(const char* fullpath_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    point_normal_vertex_t* tmp_vertices = NULL;
    fs_stream_t* fs_stream = NULL;

    triangle_data_t tmp_triangle_data = { 0 };
    size_t tmp_vertex_count = 0;
    size_t triangle_count = 0;
    size_t tmp_triangle_count = 0;
    triangle_data_block_t data_block = { 0 };

    bool block_loaded = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "stl_ascii_loader_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "stl_ascii_loader_load", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "stl_ascii_loader_load", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_BAD_OPERATION, resource_result_to_str(RESOURCE_BAD_OPERATION), "stl_ascii_loader_load", "*out_vertices_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - Provided fullpath_ is not valid.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    // 事前ロードで頂点数、三角形数を取得
    ret = vertex_count_preload(fullpath_, &tmp_triangle_count, &tmp_vertex_count);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - vertex_count_preload failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    if(!count_is_valid(tmp_triangle_count, tmp_vertex_count)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - unsupported file.", resource_result_to_str(ret));
        goto cleanup;
    }

    // リソース確保
    ret_general_allocator = general_allocator_allocate(sizeof(point_normal_vertex_t) * tmp_vertex_count, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = resource_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - general_allocator_allocate failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // fs_stream初期化
    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - fs_stream_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // ロード処理
    ret = triangle_data_block_initialize(&data_block);
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - triangle_data_block_initialize failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    while(true) {
        block_loaded = false;
        // facet...end facetまでをロード
        ret = triangle_data_block_load_next(fs_stream, &data_block, &block_loaded);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("stl_ascii_loader_load(%s) - triangle_data_block_load_next failed.", resource_result_to_str(ret));
            goto cleanup;
        }
        if(!block_loaded) {
            break;
        }
        if(triangle_count >= tmp_triangle_count) {
            ret = RESOURCE_RUNTIME_ERROR;
            goto cleanup;
        }
        if(!triangle_data_block_is_valid(&data_block)) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("stl_ascii_loader_load(%s) - Triangle data block format is not valid.", resource_result_to_str(ret));
            goto cleanup;
        }

        // facet...end facetまでをparse
        ret = triangle_data_block_parse(&data_block, &tmp_triangle_data);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("stl_ascii_loader_load(%s) - triangle_data_block_parse failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // データ形式をpoint_normal_vertex_t形式に合わせる(値の整合性チェックはCommit eligibility.で行う)
        ret = triangle_normalize(&tmp_triangle_data, &tmp_vertices[triangle_count * 3]);
        if(RESOURCE_SUCCESS != ret) {
            ERROR_MESSAGE("stl_ascii_loader_load(%s) - stl_ascii_loader_load failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        triangle_count++;
    }
    if(triangle_count != tmp_triangle_count) {
        ret = RESOURCE_RUNTIME_ERROR;
        goto cleanup;
    }
    fs_stream_destroy(&fs_stream, NULL);

    // Commit eligibility.
    if(!load_result_is_valid(tmp_vertices, tmp_vertex_count)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("stl_ascii_loader_load(%s) - Invalid vertex value.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    *out_vertex_count_ = tmp_vertex_count;
    *out_vertices_ = tmp_vertices;
    tmp_vertices = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        triangle_data_block_deinitialize(&data_block);
        if(NULL != fs_stream) {
            fs_stream_destroy(&fs_stream, NULL);
        }
        if(NULL != tmp_vertices) {
            general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
        }
    }
    return ret;
}

// ============================================================
// Loading helpers
// ============================================================
static resource_result_t vertex_count_preload(const char* fullpath_, size_t* out_triangle_count_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_stream_t* fs_stream = NULL;
    choco_string_t* string = NULL;

    size_t normal_count = 0;
    size_t vertex_count = 0;

    bool complete = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fullpath_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_preload", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_triangle_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_preload", "out_triangle_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "vertex_count_preload", "out_vertex_count_")
    if('\0' == fullpath_[0]) {
        ret = RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("vertex_count_preload(%s) - Provided fullpath_ is not valid.", resource_result_to_str(RESOURCE_INVALID_ARGUMENT));
        goto cleanup;
    }

    // Prepare.
    ret_fs_stream = fs_stream_create(&fs_stream, fullpath_, FS_OPEN_MODE_READ);
    if(FS_STREAM_SUCCESS != ret_fs_stream) {
        ret = resource_result_convert_fs_stream(ret_fs_stream);
        ERROR_MESSAGE("vertex_count_preload(%s) - fs_stream_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("vertex_count_preload(%s) - Failed to create line buffer string.", resource_result_to_str(ret));
        goto cleanup;
    }

    // vertex count, normal count
    while(!complete) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream, string);
        if(FS_STREAM_EOF == ret_fs_stream) {
            complete = true;
        } else if(FS_STREAM_SUCCESS == ret_fs_stream) {
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if((SIZE_MAX - 1) < normal_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_preload(%s) - STL vertex or normal count overflowed size_t.", resource_result_to_str(ret));
                    goto cleanup;
                }
                normal_count++;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if((SIZE_MAX - 1) < vertex_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("vertex_count_preload(%s) - STL vertex or normal count overflowed size_t.", resource_result_to_str(ret));
                    goto cleanup;
                }
                vertex_count++;
            }
        } else {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("vertex_count_preload(%s) - Failed to read ASCII STL line while counting vertices.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    // Commit.
    *out_triangle_count_ = normal_count;
    *out_vertex_count_ = vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != string) {
            choco_string_destroy(&string);
        }
        if(NULL != fs_stream) {
            fs_stream_destroy(&fs_stream, NULL);
        }
    }

    return ret;
}

// success && loaded = trueでout_data_block_に値が入る
// success && loaded = falseはstlファイルの最終行に到達
// !successの場合はI/Oエラー or UNSUPPORTED_FILE
static resource_result_t triangle_data_block_load_next(fs_stream_t* fs_stream_, triangle_data_block_t* out_data_block_, bool* out_loaded_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    fs_stream_result_t ret_fs_stream = FS_STREAM_INVALID_ARGUMENT;

    bool found_block = false;
    bool loaded = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(fs_stream_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_load_next", "fs_stream_")
    IF_ARG_NULL_GOTO_CLEANUP(out_data_block_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_load_next", "out_data_block_")
    IF_ARG_NULL_GOTO_CLEANUP(out_loaded_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_load_next", "out_loaded_")

    // Commit.
    // find "facet normal"
    while(true) {
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->facet_normal_line);
        if(FS_STREAM_EOF == ret_fs_stream) {
            break;
        } else if(FS_STREAM_SUCCESS == ret_fs_stream) {
            if(choco_string_substring_exists(choco_string_c_str(out_data_block_->facet_normal_line), "facet normal")) {
                found_block = true;
                break;
            }
        } else {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    if(found_block) {
        // load outer loop
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->outer_loop_line);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // vertex1
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->vertex_line1);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // load vertex2
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->vertex_line2);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // load vertex3
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->vertex_line3);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // load endloop
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->endloop_line);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        // load endfacet
        ret_fs_stream = fs_stream_text_file_line_read(fs_stream_, out_data_block_->endfacet_line);
        if(FS_STREAM_EOF == ret_fs_stream) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - Unsupported stl ascii format.", resource_result_to_str(ret));
            goto cleanup;
        } else if(FS_STREAM_SUCCESS != ret_fs_stream) {
            ret = resource_result_convert_fs_stream(ret_fs_stream);
            ERROR_MESSAGE("triangle_data_block_load_next(%s) - fs_stream_text_file_line_read failed.", resource_result_to_str(ret));
            goto cleanup;
        }

        loaded = true;
    }
    *out_loaded_ = loaded;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t triangle_data_block_initialize(triangle_data_block_t* out_data_block_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* facet_normal_line = NULL;
    choco_string_t* outer_loop_line = NULL;
    choco_string_t* vertex_line1 = NULL;
    choco_string_t* vertex_line2 = NULL;
    choco_string_t* vertex_line3 = NULL;
    choco_string_t* endloop_line = NULL;
    choco_string_t* endfacet_line = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_data_block_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_create", "out_data_block_")

    // Prepare.
    ret_choco_string = choco_string_default_create(&facet_normal_line);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&outer_loop_line);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&vertex_line1);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&vertex_line2);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&vertex_line3);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&endloop_line);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }
    ret_choco_string = choco_string_default_create(&endfacet_line);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = resource_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("triangle_data_block_initialize(%s) - choco_string_default_create failed.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    out_data_block_->facet_normal_line = facet_normal_line;
    out_data_block_->outer_loop_line = outer_loop_line;
    out_data_block_->vertex_line1 = vertex_line1;
    out_data_block_->vertex_line2 = vertex_line2;
    out_data_block_->vertex_line3 = vertex_line3;
    out_data_block_->endloop_line = endloop_line;
    out_data_block_->endfacet_line = endfacet_line;
    facet_normal_line = NULL;
    outer_loop_line = NULL;
    vertex_line1 = NULL;
    vertex_line2 = NULL;
    vertex_line3 = NULL;
    endloop_line = NULL;
    endfacet_line = NULL;

    ret = RESOURCE_SUCCESS;

cleanup:
    if(RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != facet_normal_line) {
            choco_string_destroy(&facet_normal_line);
        }
        if(NULL != outer_loop_line) {
            choco_string_destroy(&outer_loop_line);
        }
        if(NULL != vertex_line1) {
            choco_string_destroy(&vertex_line1);
        }
        if(NULL != vertex_line2) {
            choco_string_destroy(&vertex_line2);
        }
        if(NULL != vertex_line3) {
            choco_string_destroy(&vertex_line3);
        }
        if(NULL != endloop_line) {
            choco_string_destroy(&endloop_line);
        }
        if(NULL != endfacet_line) {
            choco_string_destroy(&endfacet_line);
        }
    }
    return ret;
}

static void triangle_data_block_deinitialize(triangle_data_block_t* data_block_) {
    if(NULL == data_block_) {
        return;
    }
    choco_string_destroy(&data_block_->facet_normal_line);
    choco_string_destroy(&data_block_->outer_loop_line);
    choco_string_destroy(&data_block_->vertex_line1);
    choco_string_destroy(&data_block_->vertex_line2);
    choco_string_destroy(&data_block_->vertex_line3);
    choco_string_destroy(&data_block_->endloop_line);
    choco_string_destroy(&data_block_->endfacet_line);
}

static resource_result_t triangle_data_block_parse(const triangle_data_block_t* data_block_, triangle_data_t* out_triangle_data_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    int parse_result = 0;
    vec3f_t tmp_normal = { 0 };
    vec3f_t tmp_vertex1 = { 0 };
    vec3f_t tmp_vertex2 = { 0 };
    vec3f_t tmp_vertex3 = { 0 };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(data_block_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_parse", "data_block_")
    IF_ARG_NULL_GOTO_CLEANUP(out_triangle_data_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_data_block_parse", "out_triangle_data_")

    // Prepare.
    parse_result = sscanf(choco_string_c_str(data_block_->facet_normal_line), "  facet normal %f %f %f", &tmp_normal.elem[0], &tmp_normal.elem[1], &tmp_normal.elem[2]);
    if(3 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("triangle_data_block_parse(%s) Invalid triangle data block.", resource_result_to_str(ret));
        goto cleanup;
    }

    parse_result = sscanf(choco_string_c_str(data_block_->vertex_line1), "      vertex %f %f %f", &tmp_vertex1.elem[0], &tmp_vertex1.elem[1], &tmp_vertex1.elem[2]);
    if(3 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("triangle_data_block_parse(%s) Invalid triangle data block.", resource_result_to_str(ret));
        goto cleanup;
    }
    parse_result = sscanf(choco_string_c_str(data_block_->vertex_line2), "      vertex %f %f %f", &tmp_vertex2.elem[0], &tmp_vertex2.elem[1], &tmp_vertex2.elem[2]);
    if(3 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("triangle_data_block_parse(%s) Invalid triangle data block.", resource_result_to_str(ret));
        goto cleanup;
    }
    parse_result = sscanf(choco_string_c_str(data_block_->vertex_line3), "      vertex %f %f %f", &tmp_vertex3.elem[0], &tmp_vertex3.elem[1], &tmp_vertex3.elem[2]);
    if(3 != parse_result) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("triangle_data_block_parse(%s) Invalid triangle data block.", resource_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    out_triangle_data_->normal = tmp_normal;
    out_triangle_data_->vertices[0] = tmp_vertex1;
    out_triangle_data_->vertices[1] = tmp_vertex2;
    out_triangle_data_->vertices[2] = tmp_vertex3;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static resource_result_t triangle_normalize(const triangle_data_t* triangle_data_, point_normal_vertex_t out_vertex_[3]) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(triangle_data_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_normalize", "triangle_data_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_, ret, RESOURCE_INVALID_ARGUMENT, resource_result_to_str(RESOURCE_INVALID_ARGUMENT), "triangle_normalize", "out_vertex_")
    if(!vec3f_is_finite(triangle_data_->normal)) {
        ret = RESOURCE_UNSUPPORTED_FILE;
        ERROR_MESSAGE("triangle_normalize(%s) - Invalid normal value.", resource_result_to_str(ret));
        goto cleanup;
    }
    for(size_t i = 0; i != 3; ++i) {
        if(-1.0f > triangle_data_->normal.elem[i] || 1.0f < triangle_data_->normal.elem[i]) {
            ret = RESOURCE_UNSUPPORTED_FILE;
            ERROR_MESSAGE("triangle_normalize(%s) - Invalid normal value.", resource_result_to_str(ret));
            goto cleanup;
        }
    }

    // Commit.
    for(size_t i = 0; i != 3; ++i) {
        out_vertex_[i].normal = vec4i8_initialize((int8_t)(127.5f * triangle_data_->normal.elem[0]), (int8_t)(127.5f * triangle_data_->normal.elem[1]), (int8_t)(127.5f * triangle_data_->normal.elem[2]), 0);
        out_vertex_[i].position = triangle_data_->vertices[i];
    }

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

static bool count_is_valid(size_t triangle_count_, size_t vertex_count_) {
    if((SIZE_MAX / 3) < triangle_count_) {
        return false;
    }
    if(0 == vertex_count_ || (triangle_count_ * 3) != vertex_count_) {
        return false;
    }
    if((SIZE_MAX / vertex_count_) < sizeof(point_normal_vertex_t)) {
        return false;
    }
    return true;
}

static bool triangle_data_block_is_valid(const triangle_data_block_t* data_block_) {
    if(NULL == data_block_) {
        return false;
    }
    if(!choco_string_substring_exists(choco_string_c_str(data_block_->facet_normal_line), "  facet normal ")) {
        return false;
    }
    if(!choco_string_is_equal(choco_string_c_str(data_block_->outer_loop_line), "    outer loop")) {
        return false;
    }
    if(!choco_string_substring_exists(choco_string_c_str(data_block_->vertex_line1), "      vertex ")) {
        return false;
    }
    if(!choco_string_substring_exists(choco_string_c_str(data_block_->vertex_line2), "      vertex ")) {
        return false;
    }
    if(!choco_string_substring_exists(choco_string_c_str(data_block_->vertex_line3), "      vertex ")) {
        return false;
    }
    if(!choco_string_is_equal(choco_string_c_str(data_block_->endloop_line), "    endloop")) {
        return false;
    }
    if(!choco_string_is_equal(choco_string_c_str(data_block_->endfacet_line), "  endfacet")) {
        return false;
    }
    return true;
}

static bool load_result_is_valid(const point_normal_vertex_t* vertices_, size_t vertex_count_) {
    if(NULL == vertices_) {
        return false;
    }
    if(0 == vertex_count_) {
        return false;
    }
    for(size_t i = 0; i != vertex_count_; ++i) {
        if(!vec3f_is_finite(vertices_[i].position)) {
            return false;
        }
    }
    return true;
}
