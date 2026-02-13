#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_SHADER_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_SHADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

/**
 * @brief シェーダー構造体インスタンスのメモリを確保し、renderer_backend_shader_tインスタンスのフィールドを全て0で初期化する
 *
 * @note shader_handle_のリソース管理は本モジュールで行うため、確保されたメモリは使用者が @ref renderer_shader_vtable_t が保有する @ref renderer_shader_destroy を呼び出して解放する
 *
 * @param[out] shader_handle_ renderer_backend_shader_t構造体インスタンスへのダブルポインタ
 *
 * @retval RENDERER_INVALID_ARGUMENT 以下のいずれか
 * - shader_handle_ == NULL
 * - *shader_handle_ != NULL
 * - メモリシステム未初期化(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_LIMIT_EXCEEDED メモリ管理システムのシステム使用可能範囲上限超過
 * @retval RENDERER_NO_MEMORY メモリ割り当て失敗
 * @retval RENDERER_UNDEFINED_ERROR 想定していない実行結果コードを処理過程で受け取った(render_mem_allocateからのエラー伝播)
 * @retval RENDERER_SUCCESS メモリ確保および初期化に成功し、正常終了
 * @retval 上記以外 グラフィックスAPI実装依存
 */
typedef renderer_result_t (*pfn_renderer_shader_create)(renderer_backend_shader_t** shader_handle_);

typedef void (*pfn_renderer_shader_destroy)(renderer_backend_shader_t** shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_compile)(shader_type_t shader_type_, const char* shader_source_, renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_link)(renderer_backend_shader_t* shader_handle_);

typedef renderer_result_t (*pfn_renderer_shader_use)(renderer_backend_shader_t* shader_handle_, uint32_t* out_program_id_);

typedef struct renderer_shader_vtable {
    pfn_renderer_shader_create renderer_shader_create;      /**< 関数ポインタ @ref pfn_renderer_shader_create参照 */
    pfn_renderer_shader_destroy renderer_shader_destroy;
    pfn_renderer_shader_compile renderer_shader_compile;
    pfn_renderer_shader_link renderer_shader_link;
    pfn_renderer_shader_use renderer_shader_use;
} renderer_shader_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
