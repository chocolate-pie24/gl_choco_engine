/**
 * @file test_lit_mesh_geometry.h
 * @author chocolate-pie24
 * @brief lit_mesh_geometryモジュール用テストAPI定義
 *
 * @version 0.1
 * @date 2026-06-04
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_RESOURCE_GEOMETRY_TEST_LIT_MESH_GEOMETRY_H
#define GLCE_TEST_ENGINE_RESOURCE_GEOMETRY_TEST_LIT_MESH_GEOMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief lit_mesh_geometry_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、lit_mesh_geometry内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_lit_mesh_geometry_create_config_set(const test_call_control_t* config_);

/**
 * @brief lit_mesh_geometry_initialize_from_vertices()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、lit_mesh_geometry内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_lit_mesh_geometry_initialize_from_vertices_config_set(const test_call_control_t* config_);

/**
 * @brief lit_mesh_geometry_initialize_from_file()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、lit_mesh_geometry内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_lit_mesh_geometry_initialize_from_file_config_set(const test_call_control_t* config_);

/**
 * @brief lit_mesh_geometry_vertices_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、lit_mesh_geometry内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_lit_mesh_geometry_vertices_get_config_set(const test_call_control_t* config_);

/**
 * @brief lit_mesh_geometry_vertex_count_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、lit_mesh_geometry内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_lit_mesh_geometry_vertex_count_get_config_set(const test_call_control_t* config_);

/**
 * @brief lit_mesh_geometryが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_lit_mesh_geometry_config_reset(void);

/**
 * @brief lit_mesh_geometry保有APIのテストを行う
 *
 */
void test_lit_mesh_geometry(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
