#ifndef GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_ERR_UTILS_H
#define GLCE_ENGINE_RESOURCE_RESOURCE_CORE_RESOURCE_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "engine/core/memory/choco_memory.h"
#include "engine/core/filesystem/filesystem.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

const char* resource_rslt_to_str(resource_result_t rslt_);

resource_result_t resource_rslt_convert_choco_memory(memory_system_result_t result_);

resource_result_t resource_rslt_convert_filesystem(filesystem_result_t result_);

resource_result_t resource_rslt_convert_fs_utils(fs_utils_result_t result_);

resource_result_t resource_rslt_convert_choco_string(choco_string_result_t result_);

#ifdef __cplusplus
}
#endif
#endif
