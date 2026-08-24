// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

/** @ingroup platform
 *
 * @file platform_glfw.h
 * @author chocolate-pie24
 * @brief GLFW APIで実装されたプラットフォームシステムAPIを提供する
 *
 * @date 2025-10-14
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONCRETES_GLFW_PLATFORM_GLFW_H
#define GLCE_ENGINE_SYSTEMS_PLATFORM_PLATFORM_CONCRETES_GLFW_PLATFORM_GLFW_H
#ifdef __cplusplus
extern "C" {
#endif

#include "engine/systems/platform/vtables/platform_vtable.h"

/**
 * @brief GLFWプラットフォームを使用する際の仮想関数テーブルを取得する
 *
 * @note vtableの詳細は @ref platform_vtable_t を参照
 *
 * @return const platform_vtable_t* 仮想関数テーブル
 */
const platform_vtable_t* platform_glfw_vtable_get(void);

#ifdef __cplusplus
}
#endif
#endif
