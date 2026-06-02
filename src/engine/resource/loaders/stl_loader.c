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
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // for sscanf
#include <math.h>   // for isfinite

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
    point_normal_vertex_t* vertices;    /**< 頂点情報格納配列 */
    size_t vertex_count;                /**< 頂点数 */
};

static resource_result_t stl_loader_vertex_count_calc(const char* path_, const char* name_, const char* extension_, size_t* out_vertex_count_);

#ifdef TEST_BUILD
#include <assert.h>
#include <string.h>

#include "test_controller.h"

#include "engine/resource/loaders/test_stl_loader.h"

#include "engine/core/memory/test_choco_memory.h"
#include "engine/core/filesystem/test_filesystem.h"

#include "engine/containers/test_choco_string.h"

#include "engine/io_utils/fs_utils/test_fs_utils.h"

// bmp_loader用モジュール専用テスト制御構造体定義

// 外部公開APIテスト設定
static test_call_control_t s_test_config_stl_loader_create;                 /**< stl_loader_create()テスト設定 */
static test_call_control_t s_test_config_stl_loader_ascii_load;             /**< stl_loader_ascii_load()テスト設定 */
static test_call_control_t s_test_config_stl_loader_vertices_move;          /**< stl_loader_vertices_move()テスト設定 */
static test_call_control_t s_test_config_stl_loader_vertices_count_get;     /**< stl_loader_vertices_count_get()テスト設定 */

// プライベート関数テスト設定
static test_call_control_t s_test_config_stl_loader_vertex_count_calc;  /**< stl_loader_vertex_count_calc()テスト設定 */

// 全テスト関数プロトタイプ宣言
static void test_stl_loader_create(void);
static void test_stl_loader_destroy(void);
static void test_stl_loader_ascii_load(void);
static void test_stl_loader_vertices_move(void);
static void test_stl_loader_vertices_count_get(void);
static void test_stl_loader_vertex_count_calc(void);

// テスト用ヘルパー関数

#endif

resource_result_t stl_loader_create(stl_loader_t** stl_loader_) {
#ifdef TEST_BUILD
    s_test_config_stl_loader_create.call_count++;
    if(s_test_config_stl_loader_create.fail_on_call != 0) {
        if(s_test_config_stl_loader_create.call_count == s_test_config_stl_loader_create.fail_on_call) {
            return (resource_result_t)s_test_config_stl_loader_create.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_stl_loader_ascii_load.call_count++;
    if(s_test_config_stl_loader_ascii_load.fail_on_call != 0) {
        if(s_test_config_stl_loader_ascii_load.call_count == s_test_config_stl_loader_ascii_load.fail_on_call) {
            return (resource_result_t)s_test_config_stl_loader_ascii_load.forced_result;
        }
    }
#endif
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

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FILESYSTEM_MODE_READ, &fs_utils);
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
                if(!isfinite(tmp_normal.elem[0]) || !isfinite(tmp_normal.elem[1]) || !isfinite(tmp_normal.elem[2])) {
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
                if(!isfinite(tmp_vertex.elem[0]) || !isfinite(tmp_vertex.elem[1]) || !isfinite(tmp_vertex.elem[2])) {
                    ret = RESOURCE_DATA_CORRUPTED;
                    ERROR_MESSAGE("stl_loader_ascii_load(%s) - Invalid vertex value at line %zu. Vertex contains NaN or infinity. line = '%s'.", resource_rslt_to_str(ret), line_count, choco_string_c_str(string));
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
#ifdef TEST_BUILD
    s_test_config_stl_loader_vertices_move.call_count++;
    if(s_test_config_stl_loader_vertices_move.fail_on_call != 0) {
        if(s_test_config_stl_loader_vertices_move.call_count == s_test_config_stl_loader_vertices_move.fail_on_call) {
            return (resource_result_t)s_test_config_stl_loader_vertices_move.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_stl_loader_vertices_count_get.call_count++;
    if(s_test_config_stl_loader_vertices_count_get.fail_on_call != 0) {
        if(s_test_config_stl_loader_vertices_count_get.call_count == s_test_config_stl_loader_vertices_count_get.fail_on_call) {
            return (resource_result_t)s_test_config_stl_loader_vertices_count_get.forced_result;
        }
    }
#endif
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
#ifdef TEST_BUILD
    s_test_config_stl_loader_vertex_count_calc.call_count++;
    if(s_test_config_stl_loader_vertex_count_calc.fail_on_call != 0) {
        if(s_test_config_stl_loader_vertex_count_calc.call_count == s_test_config_stl_loader_vertex_count_calc.fail_on_call) {
            return (resource_result_t)s_test_config_stl_loader_vertex_count_calc.forced_result;
        }
    }
#endif
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

    ret_fs_utils = fs_utils_create(path_, name_, extension_, FILESYSTEM_MODE_READ, &fs_utils);
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

#ifdef TEST_BUILD
void NO_COVERAGE test_stl_loader_create_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_stl_loader_create.fail_on_call = config_->fail_on_call;
    s_test_config_stl_loader_create.forced_result = config_->forced_result;
}

void NO_COVERAGE test_stl_loader_ascii_load_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_stl_loader_ascii_load.fail_on_call = config_->fail_on_call;
    s_test_config_stl_loader_ascii_load.forced_result = config_->forced_result;
}

void NO_COVERAGE test_stl_loader_vertices_move_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_stl_loader_vertices_move.fail_on_call = config_->fail_on_call;
    s_test_config_stl_loader_vertices_move.forced_result = config_->forced_result;
}

void NO_COVERAGE test_stl_loader_vertices_count_get_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_stl_loader_vertices_count_get.fail_on_call = config_->fail_on_call;
    s_test_config_stl_loader_vertices_count_get.forced_result = config_->forced_result;
}


void NO_COVERAGE test_stl_loader_config_reset(void) {
    test_call_control_reset(&s_test_config_stl_loader_create);
    test_call_control_reset(&s_test_config_stl_loader_ascii_load);
    test_call_control_reset(&s_test_config_stl_loader_vertices_move);
    test_call_control_reset(&s_test_config_stl_loader_vertices_count_get);

    test_call_control_reset(&s_test_config_stl_loader_vertex_count_calc);
}

void NO_COVERAGE test_stl_loader(void) {
    test_stl_loader_create();
    test_stl_loader_destroy();
    test_stl_loader_ascii_load();
    test_stl_loader_vertices_move();
    test_stl_loader_vertices_count_get();
    test_stl_loader_vertex_count_calc();
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_create(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // stl_loader_create() 冒頭で強制的に RESOURCE_NO_MEMORY を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        s_test_config_stl_loader_create.fail_on_call = 1U;
        s_test_config_stl_loader_create.forced_result = (int)RESOURCE_NO_MEMORY;

        ret = stl_loader_create(&loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *stl_loader_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t dummy_loader = { 0 };
        stl_loader_t* loader = &dummy_loader;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_loader == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // memory_system_allocate() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t* loader = NULL;
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)MEMORY_SYSTEM_NO_MEMORY;
        test_memory_system_allocate_config_set(&config);

        ret = stl_loader_create(&loader);
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: stl_loader_t が確保され、全フィールドが初期化される
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        assert(NULL == loader->vertices);
        assert(0U == loader->vertex_count);

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_destroy(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // stl_loader_ == NULL -> 何もせずreturn
        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        stl_loader_destroy(NULL);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *stl_loader_ == NULL -> 何もせずreturn
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vertices == NULL, vertex_count == 0 のloader本体だけを破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);
        assert(NULL == loader->vertices);
        assert(0U == loader->vertex_count);

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: vertices != NULL, vertex_count != 0 の場合、頂点配列とloader本体を破棄
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;
        point_normal_vertex_t* vertices = NULL;
        const size_t vertex_count = 3U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        loader->vertices = vertices;
        loader->vertex_count = vertex_count;

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 破損状態: vertices != NULL, vertex_count == 0 の場合、loader本体のみ破棄される
        // verticesはdestroy側ではfreeされない仕様なので、テスト側で後始末する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;
        point_normal_vertex_t* leaked_vertices = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY, (void**)&leaked_vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != leaked_vertices);

        loader->vertices = leaked_vertices;
        loader->vertex_count = 0U;

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        memory_system_free(leaked_vertices, sizeof(point_normal_vertex_t), MEMORY_TAG_GEOMETRY);
        leaked_vertices = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 二重destroy相当: 1回目でNULL化され、2回目は何もせずreturn
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_ascii_load(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // テスト用ASCII STLファイル生成
        FILE* fp = NULL;

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_valid_1_triangle.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_valid_2_triangles.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 -1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_normal_parse_error.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal X 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_normal_out_of_range.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 2.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_normal_nan.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal nan 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_vertex_parse_error.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex X 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_vertex_inf.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex inf 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_facet_normal.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 -1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_outer_loop.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_vertex.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_endloop.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_endfacet.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_ascii_unexpected_eof.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(0 == fclose(fp));
    }
    {
        // stl_loader_ascii_load() 自体の失敗注入
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_stl_loader_ascii_load.fail_on_call = 1U;
        s_test_config_stl_loader_ascii_load.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // path_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load(NULL, "file", ".stl", &loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load("assets/test/filesystem/", NULL, ".stl", &loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // extension_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load("assets/test/filesystem/", "file", NULL, &loader);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load("assets/test/filesystem/", "file", ".stl", NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_->vertex_count != 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        loader.vertices = NULL;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_->vertices != NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        loader.vertices = dummy_vertices;
        loader.vertex_count = 0U;

        test_stl_loader_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(dummy_vertices == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_vertex_count_calc() 失敗 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_stl_loader_vertex_count_calc.fail_on_call = 1U;
        s_test_config_stl_loader_vertex_count_calc.forced_result = (int)RESOURCE_DATA_CORRUPTED;

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 2回目のfs_utils_create()失敗 -> RESOURCE_NO_MEMORY
        // 1回目はstl_loader_vertex_count_calc()内、2回目はstl_loader_ascii_load()本体側
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 2U;
        config.forced_result = (int)FS_UTILS_NO_MEMORY;
        test_fs_utils_create_config_set(&config);

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 2回目のchoco_string_default_create()失敗 -> RESOURCE_NO_MEMORY
        // 1回目はstl_loader_vertex_count_calc()内、2回目はstl_loader_ascii_load()本体側
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 4U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_default_create_config_set(&config);

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_NO_MEMORY == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 2パス目のline_read失敗 -> RESOURCE_RUNTIME_ERROR
        // valid_1_triangleは9行 + EOF確認1回 = 10回line_readされるため、11回目を失敗させる
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 11U;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_text_file_line_read_config_set(&config);

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            &loader
        );
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 1 triangleをロードし、頂点数と値を確認する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;
        point_normal_vertex_t* vertices = NULL;
        size_t vertex_count = 0U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_1_triangle",
            ".stl",
            loader
        );
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader->vertices);
        assert(3U == loader->vertex_count);

        ret = stl_loader_vertices_move(loader, &vertices, &vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != vertices);
        assert(3U == vertex_count);
        assert(NULL == loader->vertices);
        assert(0U == loader->vertex_count);

        assert(0.0f == vertices[0].position.elem[0]);
        assert(0.0f == vertices[0].position.elem[1]);
        assert(0.0f == vertices[0].position.elem[2]);

        assert(1.0f == vertices[1].position.elem[0]);
        assert(0.0f == vertices[1].position.elem[1]);
        assert(0.0f == vertices[1].position.elem[2]);

        assert(0.0f == vertices[2].position.elem[0]);
        assert(1.0f == vertices[2].position.elem[1]);
        assert(0.0f == vertices[2].position.elem[2]);

        assert(0 == vertices[0].normal.elem[0]);
        assert(0 == vertices[0].normal.elem[1]);
        assert(127 == vertices[0].normal.elem[2]);

        assert(0 == vertices[1].normal.elem[0]);
        assert(0 == vertices[1].normal.elem[1]);
        assert(127 == vertices[1].normal.elem[2]);

        assert(0 == vertices[2].normal.elem[0]);
        assert(0 == vertices[2].normal.elem[1]);
        assert(127 == vertices[2].normal.elem[2]);

        memory_system_free(vertices, sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY);
        vertices = NULL;
        vertex_count = 0U;

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2 trianglesをロードし、vertex_count == 6 を確認する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_valid_2_triangles",
            ".stl",
            loader
        );
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader->vertices);
        assert(6U == loader->vertex_count);

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // normal parse失敗 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_normal_parse_error",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // normal範囲外 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_normal_out_of_range",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // normal NaN -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_normal_nan",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex parse失敗 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_vertex_parse_error",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex Inf -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_vertex_inf",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 未完了facet中に次のfacet normal -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_facet_normal",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // outer loopのタイミング不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_outer_loop",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertexのタイミング不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_vertex",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // endloopのタイミング不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_endloop",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // endfacetのタイミング不正 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_endfacet",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // endfacetなしでEOF -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_ascii_load(
            "assets/test/filesystem/",
            "test_stl_loader_ascii_unexpected_eof",
            ".stl",
            &loader
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(NULL == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // テスト用ASCII STLファイル削除
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_valid_1_triangle.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_valid_2_triangles.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_normal_parse_error.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_normal_out_of_range.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_normal_nan.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_vertex_parse_error.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_vertex_inf.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_facet_normal.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_outer_loop.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_vertex.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_endloop.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_endfacet.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_ascii_unexpected_eof.stl"));
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_vertices_move(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // stl_loader_vertices_move() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t* out_vertices = NULL;
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        s_test_config_stl_loader_vertices_move.fail_on_call = 1U;
        s_test_config_stl_loader_vertices_move.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = stl_loader_vertices_move(&loader, &out_vertices, &out_vertex_count);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(NULL == out_vertices);
        assert(0U == out_vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        point_normal_vertex_t* out_vertices = NULL;
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(NULL, &out_vertices, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_vertices);
        assert(0U == out_vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertices_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(&loader, NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(0U == out_vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // *out_vertices_ != NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertex = { 0 };
        point_normal_vertex_t* out_vertices = &dummy_vertex;
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(&loader, &out_vertices, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(&dummy_vertex == out_vertices);
        assert(0U == out_vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertex = { 0 };
        point_normal_vertex_t* out_vertices = NULL;
        size_t out_vertex_count = 0U;

        loader.vertices = &dummy_vertex;
        loader.vertex_count = 0U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(&loader, &out_vertices, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);
        assert(0U == out_vertex_count);

        assert(&dummy_vertex == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // stl_loader_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t* out_vertices = NULL;
        size_t out_vertex_count = 0U;

        loader.vertices = NULL;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(&loader, &out_vertices, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(NULL == out_vertices);
        assert(0U == out_vertex_count);

        assert(NULL == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        point_normal_vertex_t* out_vertices = NULL;

        loader.vertices = dummy_vertices;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertices_move(&loader, &out_vertices, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(NULL == out_vertices);

        assert(dummy_vertices == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: verticesとvertex_countの所有権が呼び出し側へ移動し、loaderは空状態になる
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        memory_system_result_t ret_mem = MEMORY_SYSTEM_INVALID_ARGUMENT;
        stl_loader_t* loader = NULL;
        point_normal_vertex_t* vertices = NULL;
        point_normal_vertex_t* out_vertices = NULL;
        size_t out_vertex_count = 0U;
        const size_t vertex_count = 3U;

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_create(&loader);
        assert(RESOURCE_SUCCESS == ret);
        assert(NULL != loader);

        ret_mem = memory_system_allocate(sizeof(point_normal_vertex_t) * vertex_count, MEMORY_TAG_GEOMETRY, (void**)&vertices);
        assert(MEMORY_SYSTEM_SUCCESS == ret_mem);
        assert(NULL != vertices);

        loader->vertices = vertices;
        loader->vertex_count = vertex_count;

        ret = stl_loader_vertices_move(loader, &out_vertices, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);

        assert(vertices == out_vertices);
        assert(vertex_count == out_vertex_count);

        assert(NULL == loader->vertices);
        assert(0U == loader->vertex_count);

        memory_system_free(out_vertices, sizeof(point_normal_vertex_t) * out_vertex_count, MEMORY_TAG_GEOMETRY);
        out_vertices = NULL;
        out_vertex_count = 0U;

        stl_loader_destroy(&loader);
        assert(NULL == loader);

        test_stl_loader_config_reset();
        test_choco_memory_config_reset();
    }

    memory_system_destroy();
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_vertices_count_get(void) {
    {
        // stl_loader_vertices_count_get() 冒頭で強制的に RESOURCE_RUNTIME_ERROR を返させる
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        size_t out_vertex_count = 999U;
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        loader.vertices = dummy_vertices;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();

        s_test_config_stl_loader_vertices_count_get.fail_on_call = 1U;
        s_test_config_stl_loader_vertices_count_get.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = stl_loader_vertices_count_get(&loader, &out_vertex_count);
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();

        ret = stl_loader_vertices_count_get(NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };

        loader.vertices = dummy_vertices;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();

        ret = stl_loader_vertices_count_get(&loader, NULL);
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        assert(dummy_vertices == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_->vertex_count == 0 -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 999U;

        loader.vertices = dummy_vertices;
        loader.vertex_count = 0U;

        test_stl_loader_config_reset();

        ret = stl_loader_vertices_count_get(&loader, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        assert(dummy_vertices == loader.vertices);
        assert(0U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // stl_loader_->vertices == NULL -> RESOURCE_BAD_OPERATION
        resource_result_t ret = RESOURCE_SUCCESS;
        stl_loader_t loader = { 0 };
        size_t out_vertex_count = 999U;

        loader.vertices = NULL;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();

        ret = stl_loader_vertices_count_get(&loader, &out_vertex_count);
        assert(RESOURCE_BAD_OPERATION == ret);
        assert(999U == out_vertex_count);

        assert(NULL == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
    {
        // 正常系: loaderが保持するvertex_countを取得する
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        stl_loader_t loader = { 0 };
        point_normal_vertex_t dummy_vertices[3] = { 0 };
        size_t out_vertex_count = 0U;

        loader.vertices = dummy_vertices;
        loader.vertex_count = 3U;

        test_stl_loader_config_reset();

        ret = stl_loader_vertices_count_get(&loader, &out_vertex_count);
        assert(RESOURCE_SUCCESS == ret);
        assert(3U == out_vertex_count);

        assert(dummy_vertices == loader.vertices);
        assert(3U == loader.vertex_count);

        test_stl_loader_config_reset();
    }
}

// Generated by ChatGPT
static void NO_COVERAGE test_stl_loader_vertex_count_calc(void) {
    assert(MEMORY_SYSTEM_SUCCESS == memory_system_create());

    {
        // テスト用ASCII STLファイル生成
        FILE* fp = NULL;

        fp = fopen("assets/test/filesystem/test_stl_loader_count_valid_1_triangle.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_count_valid_2_triangles.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 -1.0\n", fp));
        assert(EOF != fputs("    outer loop\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 1.0\n", fp));
        assert(EOF != fputs("    endloop\n", fp));
        assert(EOF != fputs("  endfacet\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_count_empty.stl", "wb");
        assert(NULL != fp);
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_count_no_vertex.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_count_vertex_mismatch.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("  facet normal 0.0 0.0 1.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));

        fp = fopen("assets/test/filesystem/test_stl_loader_count_vertex_only.stl", "wb");
        assert(NULL != fp);
        assert(EOF != fputs("solid test\n", fp));
        assert(EOF != fputs("      vertex 0.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 1.0 0.0 0.0\n", fp));
        assert(EOF != fputs("      vertex 0.0 1.0 0.0\n", fp));
        assert(EOF != fputs("endsolid test\n", fp));
        assert(0 == fclose(fp));
    }
    {
        // stl_loader_vertex_count_calc() 自体の失敗注入
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        s_test_config_stl_loader_vertex_count_calc.fail_on_call = 1U;
        s_test_config_stl_loader_vertex_count_calc.forced_result = (int)RESOURCE_RUNTIME_ERROR;

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // path_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(NULL, "file", ".stl", &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // name_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc("assets/test/filesystem/", NULL, ".stl", &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // extension_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc("assets/test/filesystem/", "file", NULL, &out_vertex_count);
        assert(RESOURCE_INVALID_ARGUMENT == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // out_vertex_count_ == NULL -> RESOURCE_INVALID_ARGUMENT
        resource_result_t ret = RESOURCE_SUCCESS;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            NULL
        );
        assert(RESOURCE_INVALID_ARGUMENT == ret);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // fs_utils_create() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)FS_UTILS_NO_MEMORY;
        test_fs_utils_create_config_set(&config);

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_NO_MEMORY == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // choco_string_default_create() 失敗 -> RESOURCE_NO_MEMORY
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 2U;
        config.forced_result = (int)CHOCO_STRING_NO_MEMORY;
        test_choco_string_default_create_config_set(&config);

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_NO_MEMORY == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // fs_utils_text_file_line_read() 失敗 -> RESOURCE_RUNTIME_ERROR
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;
        test_call_control_t config = { 0 };

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
        test_call_control_reset(&config);

        config.fail_on_call = 1U;
        config.forced_result = (int)FS_UTILS_RUNTIME_ERROR;
        test_fs_utils_text_file_line_read_config_set(&config);

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_RUNTIME_ERROR == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 1 triangle -> vertex_count == 3
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_1_triangle",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_SUCCESS == ret);
        assert(3U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 正常系: 2 triangles -> vertex_count == 6
        resource_result_t ret = RESOURCE_INVALID_ARGUMENT;
        size_t out_vertex_count = 0U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_valid_2_triangles",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_SUCCESS == ret);
        assert(6U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // 空ファイル -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_empty",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // normalはあるがvertexがない -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_no_vertex",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertex_count != normal_count * 3 -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_vertex_mismatch",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // vertexだけでnormalがない -> RESOURCE_DATA_CORRUPTED
        resource_result_t ret = RESOURCE_SUCCESS;
        size_t out_vertex_count = 999U;

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();

        ret = stl_loader_vertex_count_calc(
            "assets/test/filesystem/",
            "test_stl_loader_count_vertex_only",
            ".stl",
            &out_vertex_count
        );
        assert(RESOURCE_DATA_CORRUPTED == ret);
        assert(999U == out_vertex_count);

        test_stl_loader_config_reset();
        test_fs_utils_config_reset();
        test_filesystem_config_reset();
        test_choco_string_config_reset();
        test_choco_memory_config_reset();
    }
    {
        // テスト用ASCII STLファイル削除
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_valid_1_triangle.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_valid_2_triangles.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_empty.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_no_vertex.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_vertex_mismatch.stl"));
        assert(0 == remove("assets/test/filesystem/test_stl_loader_count_vertex_only.stl"));
    }

    memory_system_destroy();
}
#endif
