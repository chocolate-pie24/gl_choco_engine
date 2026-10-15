// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_LOADERS_CORE_LOADER_ERR_UTILS_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_LOADERS_CORE_LOADER_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_stream.h"
#include "engine/io_utils/glce_config_utility.h"

#include "engine/systems/renderer/resources/loaders/core/loader_types.h"

const char* loader_result_to_str(loader_result_t result_);

loader_result_t loader_result_convert_fs_stream(fs_stream_result_t result_);

loader_result_t loader_result_convert_general_allocator(general_allocator_result_t result_);

loader_result_t loader_result_convert_choco_string(choco_string_result_t result_);

loader_result_t loader_result_convert_glce_config_utility(glce_config_utility_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
