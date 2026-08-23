#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"

#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/resources/allocators/range_allocator.h"

bool vbo_range_is_valid(const vbo_range_t* vbo_range_) {
    if(NULL == vbo_range_) {
        return false;
    }
    if(!draw_range_is_valid(&vbo_range_->draw_range)) {
        return false;
    }
    if(!range_allocation_is_valid(&vbo_range_->allocation_info)) {
        return false;
    }
    return true;
}
