// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_LIGHT_AMBIENT_LIGHT_H
#define GLCE_ENGINE_LIGHT_AMBIENT_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "engine/base/choco_math/math_types.h"

#include "engine/light/core/light_types.h"

typedef struct ambient_light {
    vec3f_t color;      // 環境光の色(0〜１で0〜255を表現する)
    float intensity;    // 環境光の強度
} ambient_light_t;

light_result_t ambient_light_initialize(vec3f_t color_, float intensity_, ambient_light_t* out_light_);

bool ambient_light_is_valid(const ambient_light_t* light_);

#ifdef __cplusplus
}
#endif
#endif
