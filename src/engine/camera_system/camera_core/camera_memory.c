/** @ingroup camera_system
 *
 * @file camera_memory.c
 * @author chocolate-pie24
 * @brief カメラシステムレイヤー内でのメモリ確保/解放における実行結果コードと、メモリタグを統一化するため、choco_memoryモジュールのラップAPIの実装
 *
 * @version 0.1
 * @date 2026-03-19
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 */
#include <stddef.h>

#include "engine/camera_system/camera_core/camera_memory.h"

#include "engine/camera_system/camera_core/camera_err_utils.h"
#include "engine/camera_system/camera_core/camera_types.h"

#include "engine/core/memory/choco_memory.h"

camera_result_t camera_mem_allocate(size_t size_, void** out_ptr_) {
    camera_result_t ret = CAMERA_INVALID_ARGUMENT;
    memory_system_result_t ret_msys = memory_system_allocate(size_, MEMORY_TAG_CAMERA, out_ptr_);
    ret = camera_rslt_convert_choco_memory(ret_msys);

    return ret;
}

void camera_mem_free(void* ptr_, size_t size_) {
    memory_system_free(ptr_, size_, MEMORY_TAG_CAMERA);
}
