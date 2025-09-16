/**　@addtogroup core_memory
 * @{
 *
 * @file choco_memory.h
 * @author chocolate-pie24
 *
 * @version 0.1
 * @date 2025-09-16
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H
#define GLCE_ENGINE_CORE_MEMORY_CHOCO_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef enum {
    MEMORY_TAG_SYSTEM,
    MEMORY_TAG_STRING,
    MEMORY_TAG_MAX,
} memory_tag_t;

typedef enum {
    MEMORY_SYSTEM_SUCCESS,
    MEMORY_SYSTEM_INVALID_ARGUMENT,
    MEMORY_SYSTEM_RUNTIME_ERROR,
    MEMORY_SYSTEM_NO_MEMORY,
} memory_sys_err_t;

typedef struct memory_system memory_system_t;

void memory_system_preinit(size_t* memory_requirement_, size_t* alignment_requirement_);

// preinitで取得したmemory_requirement, alignment_requirementを元にアプリケーション側で確保したメモリを私、initで内部データを初期化
memory_sys_err_t memory_system_init(memory_system_t* const memory_system_);

void memory_system_destroy(memory_system_t* const memory_system_);

memory_sys_err_t memory_system_allocate(memory_system_t* const memory_system_, size_t size_, memory_tag_t mem_tag_, void** out_ptr_);

void memory_system_free(memory_system_t* const memory_system_, void* ptr_, size_t size_, memory_tag_t mem_tag_);

void memory_system_report(const memory_system_t* const memory_system_);

#ifdef __cplusplus
}
#endif
#endif

/*@}*/
