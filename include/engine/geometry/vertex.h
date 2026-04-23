#ifndef GLCE_ENGINE_GEOMETRY_VERTEX_H
#define GLCE_ENGINE_GEOMETRY_VERTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/base/choco_math/math_types.h"

/**
 * @brief ui geometry用頂点情報構造体
 *
 */
typedef struct ui_vertex {
    vec2f_t position;   /**< 頂点座標 */
    vec2f_t tex_coord; /**< テクスチャUV座標 */
} ui_vertex_t;

#ifdef __cplusplus
}
#endif
#endif
