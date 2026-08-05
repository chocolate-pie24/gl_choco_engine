/**
 * @file test_aabb_3d.h
 * @author chocolate-pie24
 * @brief aabb_3dテスト用API定義
 *
 * @version 0.1
 * @date 2026-06-09
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#ifndef GLCE_TEST_ENGINE_CORE_GEOMETRY_PRIMITIVE_TEST_AABB_3D_H
#define GLCE_TEST_ENGINE_CORE_GEOMETRY_PRIMITIVE_TEST_AABB_3D_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief aabb_3d_initialize_from_min_max()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_initialize_from_min_max_config_set(const test_call_control_t* config_);

/**
 * @brief aabb_3d_initialize_from_point_vertices()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_initialize_from_point_vertices_config_set(const test_call_control_t* config_);

/**
 * @brief aabb_3d_initialize_from_line_vertices()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_initialize_from_line_vertices_config_set(const test_call_control_t* config_);

/**
 * @brief aabb_3d_initialize_from_point_normal_vertices()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_initialize_from_point_normal_vertices_config_set(const test_call_control_t* config_);

/**
 * @brief aabb_3d_vertices_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_vertices_get_config_set(const test_call_control_t* config_);

/**
 * @brief aabb_3d_is_valid()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、aabb_3d内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_aabb_3d_is_valid_config_set(const test_call_control_bool_t* config_);

/**
 * @brief aabb_3dが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_aabb_3d_config_reset(void);

/**
 * @brief aabb_3d保有APIのテストを行う
 *
 */
void test_aabb_3d(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
