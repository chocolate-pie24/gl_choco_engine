// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_CYLINDER_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_CYLINDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct cylinder {
    float height;
    float radius;
} cylinder_t;

cylinder_t cylinder_initialize(float height_, float radius_);

bool cylinder_is_valid(const cylinder_t* cylinder_);

#ifdef __cplusplus
}
#endif
#endif
