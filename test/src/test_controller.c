/**
 * @file test_controller.c
 * @author chocolate-pie24
 * @brief 各API単体テスト時に使用モジュールの実行結果を制御するための失敗注入機能の実装
 *
 * @version 0.1
 * @date 2026-03-13
 *
 * @copyright Copyright (c) 2025 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stdint.h>

#include "test_controller.h"

void test_call_control_reset(test_call_control_t* control_) {
    control_->call_count = 0;
    control_->fail_on_call = 0;
    control_->forced_result = 0;
}

void test_call_control_set(uint32_t fail_on_call_, int forced_result_, test_call_control_t* out_control_) {
    out_control_->fail_on_call = fail_on_call_;
    out_control_->forced_result = forced_result_;
}
