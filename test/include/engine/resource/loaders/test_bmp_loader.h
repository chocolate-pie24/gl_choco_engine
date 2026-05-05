#ifndef GLCE_TEST_ENGINE_RESOURCE_LOADERS_TEST_BMP_LOADER_H
#define GLCE_TEST_ENGINE_RESOURCE_LOADERS_TEST_BMP_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

/**
 * @brief bmp_loader_create()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、bmp_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_bmp_loader_create_config_set(const test_call_control_t* config_);

/**
 * @brief bmp_loader_load()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、bmp_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_bmp_loader_load_config_set(const test_call_control_t* config_);

/**
 * @brief bmp_loader_pixel_move()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、bmp_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_bmp_loader_pixel_move_config_set(const test_call_control_t* config_);

/**
 * @brief bmp_loader_bmp_size_get()APIに対して失敗注入設定を行う
 *
 * @note API呼び出し回数についてはコピーされず、bmp_loader内で管理している値が保持される
 *
 * @param[in] config_ テスト設定値構造体インスタンスへのポインタ
 */
void test_bmp_loader_bmp_size_get_config_set(const test_call_control_t* config_);

/**
 * @brief bmp_loaderが内部で管理するテスト設定値を全て初期化し、テスト用の出力強制制御をなくす
 *
 */
void test_bmp_loader_config_reset(void);

/**
 * @brief bmp_loader保有APIのテストを行う
 *
 */
void test_bmp_loader(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
