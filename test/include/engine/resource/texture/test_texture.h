#ifndef GLCE_TEST_ENGINE_RESOURCE_TEXTURE_TEST_TEXTURE_H
#define GLCE_TEST_ENGINE_RESOURCE_TEXTURE_TEST_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief texture_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_create_config_set(const test_call_control_t* config_);

/**
 * @brief texture_pixel_load()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_pixel_load_config_set(const test_call_control_t* config_);

/**
 * @brief texture_pixel_unload()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_pixel_unload_config_set(const test_call_control_t* config_);

/**
 * @brief texture_pixel_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_pixel_get_config_set(const test_call_control_t* config_);

/**
 * @brief texture_pixel_size_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、texture内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_texture_pixel_size_get_config_set(const test_call_control_t* config_);

/**
 * @brief textureが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_texture_config_reset(void);

/**
 * @brief texture保有APIのテストを行う
 *
 */
void test_texture(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
