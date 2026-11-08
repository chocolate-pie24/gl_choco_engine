// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_CONE_H
#define GLCE_ENGINE_CORE_GEOMETRY_PRIMITIVE_CONE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct cone {
    float height;
    float radius;
} cone_t;

cone_t cone_initialize(float height_, float radius_);

bool cone_is_valid(const cone_t* cone_);

#ifdef __cplusplus
}
#endif
#endif
