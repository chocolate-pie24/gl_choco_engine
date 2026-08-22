#include "engine/systems/renderer/core/renderer_types.h"

#include <stdbool.h>

bool buffer_usage_is_valid(buffer_usage_t usage_) {
    switch(usage_) {
    case BUFFER_USAGE_DYNAMIC:
        return true;
    case BUFFER_USAGE_STATIC:
        return true;
    default:
        return false;
    }
}
