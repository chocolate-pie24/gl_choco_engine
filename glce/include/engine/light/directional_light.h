// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_LIGHT_DIRECTIONAL_LIGHT_H
#define GLCE_ENGINE_LIGHT_DIRECTIONAL_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/light/core/light_types.h"

// NOTE: directionは物体の表面から光源へ向かうベクトル
// 光源は無限遠にあり、全ての物体に対して光が平行に届くと近似する
// なお、directionの座標系はworld coordinateである
typedef struct directional_light {
    vec3f_t direction;
    vec3f_t color;
    float intensity;
} directional_light_t;

light_result_t directional_light_initialize(vec3f_t direction_, vec3f_t color_, float intensity_, directional_light_t* out_light_);

bool directional_light_is_valid(const directional_light_t* light_);

#ifdef __cplusplus
}
#endif
#endif
