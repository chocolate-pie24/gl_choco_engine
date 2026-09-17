// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_CONFIG_BUILD_CONFIG_H
#define GLCE_CONFIG_BUILD_CONFIG_H

/*
 * GLCE Build Configuration
 *
 * GLCE binary全体へ適用するcompile-time build contractを定義する。
 *
 * このファイルの設定はoverrideを前提としない。
 * runtimeまたは用途ごとに変更する値はここへ追加しない。
 *
 * configuration項目が増大したらgeneratorの追加を検討する。
 */

/* =========================================================================
 * Platform Backend
 * =========================================================================
 *
 * GLCE Platform Systemが使用するbackendを指定する。
 *
 * Available values:
 *
 *   GLCE_BUILD_PLATFORM_GLFW
 *       GLFWを使用する。
 *
 * 現在は必ず1つだけ定義する。
 */
#define GLCE_BUILD_PLATFORM_GLFW


/* =========================================================================
 * Graphics API
 * =========================================================================
 *
 * Renderer Backendが使用するGraphics APIを指定する。
 *
 * Available values:
 *
 *   GLCE_BUILD_GRAPHICS_API_GL33
 *       OpenGL 3.3を使用する。
 *
 * 現在は必ず1つだけ定義する。
 */
#define GLCE_BUILD_GRAPHICS_API_GL33


/* =========================================================================
 * Memory Policy
 * =========================================================================
 *
 * Memory Systemのbacking storage policyを指定する。
 *
 * Available values:
 *
 *   GLCE_BUILD_MEMORY_POLICY_DESKTOP
 *       process startup時にbacking storageを一括取得する。
 *       process shutdown時に解放する。
 *
 *   GLCE_BUILD_MEMORY_POLICY_EMBEDDED
 *       compile-time固定容量のstatic backing storageを使用する。
 *       Memory System自身はmalloc/freeを使用しない。
 *
 * 必ず1つだけ定義する。
 */
#define GLCE_BUILD_MEMORY_POLICY_DESKTOP


/* =========================================================================
 * Memory Pool Size
 * =========================================================================
 *
 * Memory Systemが管理するCPU-side memory poolの総容量。
 *
 * Unit:
 *   bytes
 *
 * Constraint:
 *   0より大きい値。
 */
#define GLCE_BUILD_MEMORY_POOL_SIZE (256ULL * 1024ULL * 1024ULL)


/* =========================================================================
 * Configuration validation
 * ========================================================================= */

#if !defined(GLCE_BUILD_PLATFORM_GLFW)
#error "GLCE platform backend is not configured."
#endif

#if !defined(GLCE_BUILD_GRAPHICS_API_GL33)
#error "GLCE graphics API is not configured."
#endif

#if defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP) && \
    defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
#error "Multiple GLCE memory policies are configured."
#endif

#if !defined(GLCE_BUILD_MEMORY_POLICY_DESKTOP) && \
    !defined(GLCE_BUILD_MEMORY_POLICY_EMBEDDED)
#error "GLCE memory policy is not configured."
#endif

#if !defined(GLCE_BUILD_MEMORY_POOL_SIZE)
#error "GLCE memory pool size is not configured."
#endif

#if GLCE_BUILD_MEMORY_POOL_SIZE <= 0
#error "GLCE memory pool size must be greater than zero."
#endif

#endif
