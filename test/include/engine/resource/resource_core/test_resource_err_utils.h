#ifndef GLCE_TEST_ENGINE_RESOURCE_RESOURCE_CORE_TEST_RESOURCE_ERR_UTILS_H
#define GLCE_TEST_ENGINE_RESOURCE_RESOURCE_CORE_TEST_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief resource_rslt_convert_choco_memory()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、resource_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_resource_rslt_convert_choco_memory_config_set(const test_call_control_t* config_);

/**
 * @brief resource_rslt_convert_filesystem()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、resource_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_resource_rslt_convert_filesystem_config_set(const test_call_control_t* config_);

/**
 * @brief resource_rslt_convert_fs_utils()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、resource_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_resource_rslt_convert_fs_utils_config_set(const test_call_control_t* config_);

/**
 * @brief resource_rslt_convert_choco_string()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、resource_err_utils内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_resource_rslt_convert_choco_string_config_set(const test_call_control_t* config_);

/**
 * @brief resource_err_utilsが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_resource_err_utils_config_reset(void);

/**
 * @brief resource_err_utils保有APIのテストを行う
 *
 */
void test_resource_err_utils(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
