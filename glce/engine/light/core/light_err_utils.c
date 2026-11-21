// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/light/core/light_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/light/core/light_types.h"

const char* light_result_to_str(light_result_t result_) {
    switch(result_) {
    case LIGHT_SUCCESS:
        return STRINGIFY(LIGHT_SUCCESS);
    case LIGHT_INVALID_ARGUMENT:
        return STRINGIFY(LIGHT_INVALID_ARGUMENT);
    case LIGHT_RUNTIME_ERROR:
        return STRINGIFY(LIGHT_RUNTIME_ERROR);
    case LIGHT_BAD_OPERATION:
        return STRINGIFY(LIGHT_BAD_OPERATION);
    case LIGHT_DATA_CORRUPTED:
        return STRINGIFY(LIGHT_DATA_CORRUPTED);
    case LIGHT_UNDEFINED_ERROR:
        return STRINGIFY(LIGHT_UNDEFINED_ERROR);
    default:
        return STRINGIFY(LIGHT_UNDEFINED_ERROR);
    }
}
