// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup resource
 *
 * @file stl_loader.c
 * @author chocolate-pie24
 * @brief STLファイルのロード処理を行うAPIの実装
 *
 * @details 以下のSTLファイルをサポートする
 * - ASCII形式のSTL(BINARY形式は将来的にサポート予定)
 * - ファイルに含まれる法線情報は[-1.0...1.0]の範囲に正規化されていること
 *
 * @todo 以下を行う
 * - GLCEカスタムフォーマットでの出力機能
 * - カスタムフォーマットが存在する場合はそちらで読み込み、ない場合は通常STLを読み込みカスタムフォーマットファイルを出力
 *
 * @date 2026-06-02
 *
 */
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

#include "engine/io_utils/fs_utils.h"

#include "engine/resource/core/resource_types.h"
#include "engine/resource/core/resource_err_utils.h"

// #define TEST_BUILD

/**
 * @brief STLファイルローダー内部情報管理構造体
 *
 */
struct stl_loader {
    point_normal_vertex_t* vertices;    /**< 頂点情報格納配列 */
    size_t vertex_count;                /**< 頂点数 */
};

static resource_result_t stl_loader_vertex_count_calc(const char* path_, const char* name_, const char* extension_, size_t* out_vertex_count_);

resource_result_t stl_loader_create(stl_loader_t** stl_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;

    stl_loader_t* tmp_stl_loader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_create", "stl_loader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_create", "*stl_loader_")

    ret_mem = memory_system_allocate(sizeof(stl_loader_t), MEMORY_TAG_GEOMETRY, (void**)&tmp_stl_loader);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("stl_loader_create(%s) - Failed to allocate stl_loader_t instance.", resource_rslt_to_str(ret));
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
    // 壊れたデータの場合、verticesがメモリリークとなるが、freeするサイズが不明であるためfreeしない(ワーニングを出す)。
    if(NULL != (*stl_loader_)->vertices && 0 != (*stl_loader_)->vertex_count) {
        memory_system_free((*stl_loader_)->vertices, sizeof(point_normal_vertex_t) * (*stl_loader_)->vertex_count, MEMORY_TAG_GEOMETRY);
        (*stl_loader_)->vertices = NULL;
        (*stl_loader_)->vertex_count = 0;
    } else if(NULL != (*stl_loader_)->vertices && 0 == (*stl_loader_)->vertex_count) {
        ERROR_MESSAGE("stl_loader_destroy(%s) - stl_loader internal state is inconsistent: vertices is not NULL but vertex_count is 0. Vertex buffer was not freed because allocation size is unknown.", resource_rslt_to_str(RESOURCE_DATA_CORRUPTED));
    }

    memory_system_free(*stl_loader_, sizeof(stl_loader_t), MEMORY_TAG_GEOMETRY);
    *stl_loader_ = NULL;
}

resource_result_t stl_loader_ascii_load(const char* path_, const char* name_, const char* extension_, stl_loader_t* stl_loader_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
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

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_ascii_load", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_ascii_load", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_ascii_load", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_ascii_load", "stl_loader_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 == stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_ascii_load", "stl_loader_->vertex_count")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_ascii_load", "stl_loader_->vertices")

    ret = stl_loader_vertex_count_calc(path_, name_, extension_, &vertex_count);
    if(0 == vertex_count) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - ASCII STL has no vertices.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to calculate vertex count for ASCII STL file.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    if((SIZE_MAX / vertex_count) < sizeof(point_normal_vertex_t)) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Vertex buffer size overflow. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(point_normal_vertex_t));
        goto cleanup;
    }
    ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&tmp_vertices);
    if(MEMORY_SYSTEM_SUCCESS != ret_mem) {
        ret = resource_rslt_convert_choco_memory(ret_mem);
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to allocate vertex buffer. vertex_count = %zu, vertex_size = %zu.", resource_rslt_to_str(ret), vertex_count, sizeof(point_normal_vertex_t));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FS_OPEN_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to create fs_utils for ASCII STL file reading.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to create line buffer string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_utils = fs_utils_text_file_line_read(fs_utils, string);
        if(FS_UTILS_EOF == ret_fs_utils) {
            complete = true;
        } else if(FS_UTILS_SUCCESS == ret_fs_utils) {
            if((SIZE_MAX - 1) < line_count) {
                ret = RESOURCE_OVERFLOW;
                ERROR_MESSAGE("stl_loader_ascii_load(%s) - ASCII STL line count overflowed size_t while loading.", resource_rslt_to_str(ret));
                goto cleanup;
            }
            line_count++;
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if(facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {  // データ不整合チェック
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected 'facet normal' at line %zu. Previous facet is incomplete. Expected 'endfacet' before starting a new facet.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                parse_result = sscanf(choco_string_c_str(string), "  facet normal %f %f %f", &tmp_normal.elem[0], &tmp_normal.elem[1], &tmp_normal.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to parse 'facet normal' at line %zu. Expected 3 float values. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                if(!vec3f_is_finite(tmp_normal)) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Invalid normal value at line %zu. Normal contains NaN or infinity. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                } else if(-1.0f > tmp_normal.elem[0] || -1.0f > tmp_normal.elem[1] || -1.0f > tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Invalid normal value at line %zu. Each normal component must be in [-1.0, 1.0]. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                } else if(1.0f < tmp_normal.elem[0] || 1.0f < tmp_normal.elem[1] || 1.0f < tmp_normal.elem[2]) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Invalid normal value at line %zu. Each normal component must be in [-1.0, 1.0]. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                facet_normal_loaded = true;
                facet_vertex_count = 0;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "outer loop")) {
                if(!facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected 'outer loop' at line %zu. Expected a valid 'facet normal' before 'outer loop'.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                outer_loop_loaded = true;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if(!facet_normal_loaded || !outer_loop_loaded || facet_vertices_loaded || endloop_loaded) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected 'vertex' at line %zu. Expected 'facet normal' and 'outer loop' before vertex lines, and expected no completed vertex list before 'endloop'.", resource_rslt_to_str(ret), line_count);
                    goto cleanup;
                }
                parse_result = sscanf(choco_string_c_str(string), "      vertex %f %f %f", &tmp_vertex.elem[0], &tmp_vertex.elem[1], &tmp_vertex.elem[2]);
                if(3 != parse_result) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to parse 'vertex' at line %zu. Expected 3 float values. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
                    goto cleanup;
                }
                if(vertex_index >= vertex_count) {  // 頂点数読み込みと現在との間にファイル状態変化
                    ret = RESOURCE_RUNTIME_ERROR;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Vertex index exceeded precomputed vertex count at line %zu. The STL file may have changed during loading, or the count pass and parse pass are inconsistent. vertex_index = %zu, vertex_count = %zu.", resource_rslt_to_str(ret), line_count, vertex_index, vertex_count);
                    goto cleanup;
                }
                if(!vec3f_is_finite(tmp_vertex)) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Invalid vertex value at line %zu. Vertex contains NaN or infinity. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
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
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected 'endloop' at line %zu. Expected exactly 3 vertex lines before 'endloop'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
                    goto cleanup;
                }
                endloop_loaded = true;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "endfacet")) {
                if(!facet_normal_loaded || !outer_loop_loaded || !facet_vertices_loaded || !endloop_loaded || 3 != facet_vertex_count) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected 'endfacet' at line %zu. Expected 'facet normal', 'outer loop', 3 vertices, and 'endloop' before 'endfacet'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
                    goto cleanup;
                }
                facet_normal_loaded = false;
                outer_loop_loaded = false;
                facet_vertices_loaded = false;
                endloop_loaded = false;
                facet_vertex_count = 0;
            }
        } else {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("stl_loader_ascii_load(%s) - Failed to read ASCII STL line. expected_line = %zu.", resource_rslt_to_str(ret), line_count + 1);
            goto cleanup;
        }
    }

    if(facet_normal_loaded || outer_loop_loaded || facet_vertices_loaded || endloop_loaded || 0 != facet_vertex_count) {    // endfacetがないままデータ終了
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Unexpected EOF while parsing ASCII STL facet. Last successfully read line = %zu. The current facet was not closed by 'endfacet'. facet_vertex_count = %zu.", resource_rslt_to_str(ret), line_count, facet_vertex_count);
        goto cleanup;
    }
    if(vertex_count != vertex_index) {
        ret = RESOURCE_RUNTIME_ERROR;
        ERROR_MESSAGE("stl_loader_ascii_load(%s) - Loaded vertex count does not match precomputed vertex count. loaded = %zu, expected = %zu.", resource_rslt_to_str(ret), vertex_index, vertex_count);
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

resource_result_t stl_loader_vertices_move(stl_loader_t* stl_loader_, point_normal_vertex_t** out_vertices_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_move", "stl_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_move", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_move", "*out_vertices_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertices_move", "stl_loader_->vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertices_move", "stl_loader_->vertices")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_move", "out_vertex_count_")

    *out_vertices_ = stl_loader_->vertices;
    *out_vertex_count_ = stl_loader_->vertex_count;

    stl_loader_->vertices = NULL;
    stl_loader_->vertex_count = 0;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

resource_result_t stl_loader_vertices_count_get(const stl_loader_t* stl_loader_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_count_get", "stl_loader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertices_count_get", "out_vertex_count_")
    IF_ARG_FALSE_GOTO_CLEANUP(0 != stl_loader_->vertex_count, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertices_count_get", "stl_loader_->vertex_count")
    IF_ARG_NULL_GOTO_CLEANUP(stl_loader_->vertices, ret, RESOURCE_BAD_OPERATION, resource_rslt_to_str(RESOURCE_BAD_OPERATION), "stl_loader_vertices_count_get", "stl_loader_->vertices")

    *out_vertex_count_ = stl_loader_->vertex_count;

    ret = RESOURCE_SUCCESS;

cleanup:
    return ret;
}

/**
 * @brief STLデータのロードに先立ち、頂点数をカウントする
 *
 * @param[in] path_ STLファイルが格納されているパス(最後は'/'が入っていること)
 * @param[in] name_ STLファイル名(拡張子は含まない)
 * @param[in] extension_ STLファイル拡張子('.'で始まること)
 * @param[out] out_vertex_count_ 頂点数格納先
 *
 * @retval RESOURCE_INVALID_ARGUMENT 以下のいずれか
 * - path_ == NULL
 * - name_ == NULL
 * - extension_ == NULL
 * @retval RESOURCE_LIMIT_EXCEEDED メモリシステム使用可能範囲上限超過
 * @retval RESOURCE_NO_MEMORY メモリ確保失敗
 * @retval RESOURCE_OVERFLOW 以下のいずれか
 * - ファイルフルパス文字列が長すぎる
 * - STLデータに格納されている頂点の数または法線の数がSIZE_MAXを超過
 * @retval RESOURCE_DATA_CORRUPTED 以下のいずれか
 * - 内部データ破損
 * - STLデータ不整合(頂点数が法線数の3倍ではない)
 * @retval RESOURCE_UNDEFINED_ERROR ファイル読み込み時に不明なエラーが発生
 * @retval RESOURCE_BAD_OPERATION メモリシステム未初期化
 * @retval RESOURCE_FILE_OPEN_ERROR STLファイルオープン失敗
 * @retval RESOURCE_RUNTIME_ERROR ファイル読み込みでエラー発生
 * @retval RESOURCE_SUCCESS 処理に成功し、正常終了
 */
static resource_result_t stl_loader_vertex_count_calc(const char* path_, const char* name_, const char* extension_, size_t* out_vertex_count_) {
    resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* string = NULL;

    size_t line_count = 0;
    size_t normal_count = 0;
    size_t vertex_count = 0;

    bool complete = false;

    IF_ARG_NULL_GOTO_CLEANUP(path_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_calc", "fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_calc", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_calc", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, RESOURCE_INVALID_ARGUMENT, resource_rslt_to_str(RESOURCE_INVALID_ARGUMENT), "stl_loader_vertex_count_calc", "out_vertex_count_")

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FS_OPEN_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = resource_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - Failed to open ASCII STL file via fs_utils.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_string = choco_string_default_create(&string);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - Failed to create line buffer string.", resource_rslt_to_str(ret));
        goto cleanup;
    }

    while(!complete) {
        ret_fs_utils = fs_utils_text_file_line_read(fs_utils, string);
        if(FS_UTILS_EOF == ret_fs_utils) {
            complete = true;
        } else if(FS_UTILS_SUCCESS == ret_fs_utils) {
            if((SIZE_MAX - 1) < line_count) {
                ret = RESOURCE_OVERFLOW;
                ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - ASCII STL line count overflowed size_t while loading.", resource_rslt_to_str(ret));
                goto cleanup;
            }
            line_count++;
            if(choco_string_substring_exists(choco_string_c_str(string), "facet normal")) {
                if((SIZE_MAX - 1) < normal_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                normal_count++;
            } else if(choco_string_substring_exists(choco_string_c_str(string), "vertex")) {
                if((SIZE_MAX - 1) < vertex_count) {
                    ret = RESOURCE_OVERFLOW;
                    ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
                    goto cleanup;
                }
                vertex_count++;
            }
        } else {
            ret = resource_rslt_convert_fs_utils(ret_fs_utils);
            ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - Failed to read ASCII STL line while counting vertices. expected_line = %zu.", resource_rslt_to_str(ret), line_count + 1);
            goto cleanup;
        }
    }

    if((SIZE_MAX / 3) < normal_count) {
        ret = RESOURCE_OVERFLOW;
        ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - STL vertex or normal count overflowed size_t.", resource_rslt_to_str(ret));
        goto cleanup;
    }
    if(0 == vertex_count || (normal_count * 3) != vertex_count) {
        ret = RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("stl_loader_vertex_count_calc(%s) - Invalid ASCII STL structure. vertex_count must be 3 times normal_count and greater than 0. normal_count = %zu, vertex_count = %zu.", resource_rslt_to_str(ret), normal_count, vertex_count);
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
