#include "engine/systems/renderer/config/renderer_config.h"

#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"

bool renderer_config_is_valid(const renderer_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->line_mesh_shader_vbo_size || 0 == config_->lit_mesh_shader_vbo_size || 0 == config_->point_mesh_shader_vbo_size || 0 == config_->ui_mesh_shader_vbo_size) {
        return false;
    }
    if(0 == config_->line_mesh_shader_max_allocation_count || 0 == config_->lit_mesh_shader_max_allocation_count || 0 == config_->point_mesh_shader_max_allocation_count || 0 == config_->ui_mesh_shader_max_allocation_count) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->line_mesh_shader_buffer_usage) || !buffer_usage_is_valid(config_->lit_mesh_shader_buffer_usage) || !buffer_usage_is_valid(config_->point_mesh_shader_buffer_usage) || ! buffer_usage_is_valid(config_->ui_mesh_shader_buffer_usage)) {
        return false;
    }
    return true;
}
