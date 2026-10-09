// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/material/material_types.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stddef.h>

#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

bool untextured_material_is_valid(const untextured_material_t* material_) {
    if(NULL == material_) {
        return false;
    }

    if(!vec3f_is_finite(material_->ambient) || !vec3f_is_finite(material_->diffuse) || !vec3f_is_finite(material_->specular)) {
        return false;
    }

    for(size_t i = 0; i != 3; ++i) {
        if(material_->ambient.elem[i] < 0.0f || material_->ambient.elem[i] > 1.0f) {
            return false;
        }
        if(material_->diffuse.elem[i] < 0.0f || material_->diffuse.elem[i] > 1.0f) {
            return false;
        }
        if(material_->specular.elem[i] < 0.0f || material_->specular.elem[i] > 1.0f) {
            return false;
        }
    }

    if(!isfinite(material_->shininess) ||
       material_->shininess < 0.0f) {
        return false;
    }

    return true;
}

bool texture_map_is_valid(const texture_map_t* material_) {
    (void)material_;
    return true;
}

bool textured_material_is_valid(const textured_material_t* material_) {
    (void)material_;
    return true;
}
