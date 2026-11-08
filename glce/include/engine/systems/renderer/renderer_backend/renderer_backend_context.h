// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @ingroup renderer
 *
 * @file renderer_backend_context.h
 * @author chocolate-pie24
 * @brief renderer_backend内部情報管理構造体のリソース管理APIを提供する
 *
 * @date 2026-02-23
 *
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

typedef struct renderer_backend_context renderer_backend_context_t; /**< renderer_backend_context内部情報管理構造体前方宣言 */

typedef struct subsystem_allocator subsystem_allocator_t;

/**
 * @brief Renderer Backend Contextを生成する
 *
 * @details
 * Subsystem AllocatorからRenderer Backend Contextのメモリを確保し、
 * ビルド時に選択されたGraphics APIに対応する以下のvtableを取得する。
 *
 * - Shader vtable
 * - VAO vtable
 * - VBO vtable
 * - Texture vtable
 *
 * 現在の実装ではOpenGL 3.3に対応するvtableを使用する。
 *
 * @note Backend ContextのメモリはSubsystem Allocatorが管理する。
 * 本APIによって個別に解放可能なメモリ領域が生成されるわけではない。
 *
 * @note vtable取得に失敗した場合、生成途中のBackend Contextを
 * 個別に解放する処理は行わない。
 *
 * @param[in] allocator_ Backend Contextのメモリ確保に使用するSubsystem Allocator
 * @param[out] out_backend_context_ 生成されたBackend Contextを格納するダブルポインタ
 *
 * @retval RENDERER_BACKEND_SUCCESS Backend Contextの生成に成功
 * @retval RENDERER_BACKEND_INVALID_ARGUMENT 以下のいずれか
 * - allocator_がNULL
 * - out_backend_context_がNULL
 * - *out_backend_context_が非NULL
 * - Subsystem AllocatorがINVALID_ARGUMENTを返した
 * @retval RENDERER_BACKEND_RUNTIME_ERROR vtableの取得に失敗
 * @retval RENDERER_BACKEND_NO_MEMORY Subsystem Allocatorのメモリ確保に失敗
 * @retval RENDERER_BACKEND_BAD_OPERATION Subsystem Allocatorが操作条件違反を検出
 * @retval RENDERER_BACKEND_DATA_CORRUPTED Subsystem Allocatorが内部データ破損を検出
 * @retval RENDERER_BACKEND_LIMIT_EXCEEDED Subsystem Allocatorの管理上限を超過
 * @retval RENDERER_BACKEND_OVERFLOW Subsystem Allocatorがオーバーフローを検出
 * @retval RENDERER_BACKEND_UNDEFINED_ERROR Subsystem Allocatorが未定義のResultを返した
 */
renderer_backend_result_t renderer_backend_create(subsystem_allocator_t* allocator_, renderer_backend_context_t** out_backend_context_);

/**
 * @brief Renderer Backend Contextの終了処理を行う
 *
 * @details
 * 現在の実装では、Backend Context固有の終了処理は行わない。
 *
 * Backend ContextはSubsystem Allocatorからメモリを取得しており、
 * 本関数でメモリを個別解放しない。
 *
 * @note backend_context_がNULLの場合も何も行わない。
 *
 * @note 本関数はBackend Contextが保持するvtableを変更しない。
 * また、Shader、VAO、VBO、TextureのGPUリソースも破棄しない。
 *
 * @param[in,out] backend_context_ 終了対象のRenderer Backend Context
 */
void renderer_backend_deinitialize(renderer_backend_context_t* backend_context_);

/**
 * @brief Renderer Backend Contextの妥当性を検証する
 *
 * @details
 * Backend Contextが保持する各vtableについて、
 * ビルド時に選択されたGraphics APIのvtableと一致することを確認する。
 *
 * 以下のvtableを検証する。
 *
 * - Shader vtable
 * - VAO vtable
 * - VBO vtable
 * - Texture vtable
 *
 * 各vtableの取得結果がNULLではないこと、
 * およびBackend Contextが保持するポインタと一致することを検証する。
 *
 * @note 本関数はvtableの対応関係を検証する。
 * Backend Context自身のAllocation Liveness、
 * GPUリソースの存在やOpenGLの実行状態は検証しない。
 *
 * @pre backend_context_が非NULLの場合、
 * 読み取り可能な有効期間中のBackend Contextを指していること
 *
 * @param[in] backend_context_ 検証対象のRenderer Backend Context
 *
 * @retval true Backend Contextが妥当
 * @retval false 以下のいずれか
 * - backend_context_がNULL
 * - Graphics APIに対応するvtableの取得結果がNULL
 * - Backend Contextが保持するvtableと取得したvtableが一致しない
 */
bool renderer_backend_is_valid(const renderer_backend_context_t* backend_context_);

#ifdef __cplusplus
}
#endif
#endif
