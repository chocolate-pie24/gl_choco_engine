/**
 * @file test_stl_loader.h
 * @author chocolate-pie24
 * @brief stl_loaderモジュール用テストAPI定義
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
#ifndef GLCE_TEST_ENGINE_RESOURCE_LOADERS_TEST_STL_LOADER_H
#define GLCE_TEST_ENGINE_RESOURCE_LOADERS_TEST_STL_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief stl_loader_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、stl_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_stl_loader_create_config_set(const test_call_control_t* config_);

/**
 * @brief stl_loader_ascii_load()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、stl_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_stl_loader_ascii_load_config_set(const test_call_control_t* config_);

/**
 * @brief stl_loader_vertices_move()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、stl_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_stl_loader_vertices_move_config_set(const test_call_control_t* config_);

/**
 * @brief stl_loader_vertices_count_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、stl_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_stl_loader_vertices_count_get_config_set(const test_call_control_t* config_);

/**
 * @brief stl_loaderが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_stl_loader_config_reset(void);

/**
 * @brief stl_loader保有APIのテストを行う
 *
 */
void test_stl_loader(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
