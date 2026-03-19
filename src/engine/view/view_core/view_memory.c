#include <stddef.h>

#include "engine/view/view_core/view_memory.h"

#include "engine/view/view_core/view_err_utils.h"
#include "engine/view/view_core/view_types.h"

#include "engine/core/memory/choco_memory.h"

view_result_t view_mem_allocate(size_t size_, void** out_ptr_) {
    view_result_t ret = VIEW_INVALID_ARGUMENT;
    memory_system_result_t ret_msys = memory_system_allocate(size_, MEMORY_TAG_VIEW, out_ptr_);
    ret = view_rslt_convert_choco_memory(ret_msys);

    return ret;
}

void view_mem_free(void* ptr_, size_t size_) {
    memory_system_free(ptr_, size_, MEMORY_TAG_VIEW);
}
