// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/fonts/core/font_err_utils.h"

#include "engine/base/choco_macros.h"

#include "engine/systems/renderer/resources/fonts/core/font_types.h"

const char* font_result_to_str(font_result_t result_) {
    switch(result_) {
    case FONT_SUCCESS:
        return STRINGIFY(FONT_SUCCESS);
    case FONT_NO_MEMORY:
        return STRINGIFY(FONT_NO_MEMORY);
    case FONT_RUNTIME_ERROR:
        return STRINGIFY(FONT_RUNTIME_ERROR);
    case FONT_INVALID_ARGUMENT:
        return STRINGIFY(FONT_INVALID_ARGUMENT);
    case FONT_DATA_CORRUPTED:
        return STRINGIFY(FONT_DATA_CORRUPTED);
    case FONT_BAD_OPERATION:
        return STRINGIFY(FONT_BAD_OPERATION);
    case FONT_OVERFLOW:
        return STRINGIFY(FONT_OVERFLOW);
    case FONT_LIMIT_EXCEEDED:
        return STRINGIFY(FONT_LIMIT_EXCEEDED);
    case FONT_UNDEFINED_ERROR:
        return STRINGIFY(FONT_UNDEFINED_ERROR);
    default:
        return STRINGIFY(FONT_UNDEFINED_ERROR);
    }
}

font_result_t font_result_convert_texture(texture_result_t result_) {
    switch(result_) {
    case TEXTURE_SUCCESS:
        return FONT_SUCCESS;
    case TEXTURE_INVALID_ARGUMENT:
        return FONT_INVALID_ARGUMENT;
    case TEXTURE_RUNTIME_ERROR:
        return FONT_RUNTIME_ERROR;
    case TEXTURE_NO_MEMORY:
        return FONT_NO_MEMORY;
    case TEXTURE_LIMIT_EXCEEDED:
        return FONT_LIMIT_EXCEEDED;
    case TEXTURE_BAD_OPERATION:
        return FONT_BAD_OPERATION;
    case TEXTURE_DATA_CORRUPTED:
        return FONT_DATA_CORRUPTED;
    case TEXTURE_OVERFLOW:
        return FONT_OVERFLOW;
    case TEXTURE_UNDEFINED_ERROR:
        return FONT_UNDEFINED_ERROR;
    default:
        return FONT_UNDEFINED_ERROR;
    }
}
