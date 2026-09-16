// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup application
 *
 * @file application.h
 * @author chocolate-pie24
 * @brief プロジェクトの最上位レイヤーで全サブシステムのオーケストレーションを行うAPIの定義
 * @details 以下の機能を提供する
 * - 全サブシステムの起動、終了処理
 * - アプリケーションメインループ
 * @details システムの起動時から終了時まで常駐
 *
 * @date 2025-09-20
 *
 */
#ifndef GLCE_APPLICATION_APPLICATION_H
#define GLCE_APPLICATION_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "application/core/application_types.h"

application_result_t application_create(void);

/**
 * @brief エンジンを構成するサブシステムを停止し、アプリケーション終了する
 *
 */
void application_destroy(void);

application_result_t application_run(void);

#ifdef __cplusplus
}
#endif
#endif
