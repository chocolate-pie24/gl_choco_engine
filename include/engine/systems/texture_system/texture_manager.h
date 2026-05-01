#ifndef GLCE_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEXTURE_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_TEXTURE_SYSTEM_TEXTURE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "engine/resource/resource_core/resource_types.h"
#include "engine/resource/texture/texture.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/renderer_backend_context.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_texture.h"

typedef struct texture_manager texture_manager_t;

#define INVALID_TEXTURE_ID (-1)

typedef enum {
    TEXTURE_SYSTEM_SUCCESS = 0,        /**< 処理成功 */
    TEXTURE_SYSTEM_NO_MEMORY,          /**< メモリ不足 */
    TEXTURE_SYSTEM_RUNTIME_ERROR,      /**< 実行時エラー */
    TEXTURE_SYSTEM_INVALID_ARGUMENT,   /**< 引数異常 */
    TEXTURE_SYSTEM_DATA_CORRUPTED,     /**< メモリ破壊, 未初期化 */
    TEXTURE_SYSTEM_BAD_OPERATION,      /**< API誤用 */
    TEXTURE_SYSTEM_OVERFLOW,           /**< 計算過程でオーバーフロー発生 */
    TEXTURE_SYSTEM_LIMIT_EXCEEDED,     /**< システム使用可能範囲上限超過 */
    TEXTURE_SYSTEM_FILE_OPEN_ERROR,    /**< ファイルオープン失敗 */
    TEXTURE_SYSTEM_FILE_READ_ERROR,    /**< ファイル読み込み失敗 */
    TEXTURE_SYSTEM_UNSUPPORTED_FILE,   /**< 未対応ファイル形式 */
    TEXTURE_SYSTEM_UNDEFINED_ERROR,    /**< 未定義エラー */
} texture_system_result_t;

// NOTE: テクスチャ配列はresizeできた方が良いので、choco_memoryでのallocateをそのうち検討する
texture_system_result_t texture_manager_initialize(int16_t max_texture_count_, linear_alloc_t* allocator_, texture_manager_t** out_texture_manager_);

void texture_manager_deinitialize(renderer_backend_context_t* backend_context_, texture_manager_t* texture_manager_);

texture_system_result_t texture_manager_register(renderer_backend_context_t* backend_context_, int32_t gpu_unit_num_, const char* texture_name_, texture_manager_t* texture_manager_, int16_t* out_texture_id_);

// texture_system_result_t texture_manager_unregister(int16_t texture_id_, texture_manager_t* texture_manager_);

// texture_system_result_t texture_manager_unregister_by_name(const char* name_, texture_manager_t* texture_manager_);

// int16_t texture_manager_texture_id_get(const char* name_, const texture_manager_t* texture_manager_);

texture_system_result_t texture_manager_gpu_resource_get(int16_t texture_id_, const texture_manager_t* texture_manager_, renderer_backend_texture_t** out_gpu_resource_);

// texture_system_result_t texture_manager_texture_get_by_name(const char* name_, const texture_manager_t* texture_manager_, texture_t** out_texture_);

#ifdef __cplusplus
}
#endif
#endif
