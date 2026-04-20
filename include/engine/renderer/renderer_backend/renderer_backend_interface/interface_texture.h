#ifndef GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_TEXTURE_H
#define GLCE_ENGINE_RENDERER_RENDERER_BACKEND_RENDERER_BACKEND_INTERFACE_INTERFACE_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "engine/renderer/renderer_backend/renderer_backend_types.h"
#include "engine/renderer/renderer_core/renderer_types.h"

typedef renderer_result_t (*pfn_renderer_texture_create)(texture_type_t type_, renderer_backend_texture_t** texture_handle_);

typedef void (*pfn_renderer_texture_destroy)(renderer_backend_texture_t** texture_handle_);

typedef renderer_result_t (*pfn_renderer_texture_bind)(const renderer_backend_texture_t* texture_handle_, int32_t* out_texture_internal_handle_);

typedef renderer_result_t (*pfn_renderer_texture_unbind)(const renderer_backend_texture_t* texture_handle_);

typedef renderer_result_t (*pfn_renderer_texture_pixel_upload)(const uint8_t* pixels_, const renderer_backend_texture_t* texture_handle_);

typedef struct renderer_texture_vtable {
    pfn_renderer_texture_create renderer_texture_create;              /**< 関数ポインタ @ref pfn_renderer_texture_create 参照 */
    pfn_renderer_texture_destroy renderer_texture_destroy;            /**< 関数ポインタ @ref pfn_renderer_texture_destroy 参照 */
    pfn_renderer_texture_bind renderer_texture_bind;                  /**< 関数ポインタ @ref pfn_renderer_texture_bind 参照 */
    pfn_renderer_texture_unbind renderer_texture_unbind;              /**< 関数ポインタ @ref pfn_renderer_texture_unbind 参照 */
    pfn_renderer_texture_pixel_upload renderer_texture_pixel_upload;  /**< 関数ポインタ @ref pfn_renderer_texture_pixel_upload 参照 */
} renderer_texture_vtable_t;

#ifdef __cplusplus
}
#endif
#endif
