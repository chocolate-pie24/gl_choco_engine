// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/config/renderer_config.h"

#include <stdbool.h>
#include <stddef.h>

#include "engine/systems/renderer/core/renderer_types.h"

bool line_mesh_shader_config_is_valid(const line_mesh_shader_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_allocation_count) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool lit_mesh_shader_config_is_valid(const lit_mesh_shader_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_allocation_count) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool point_mesh_shader_config_is_valid(const point_mesh_shader_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_allocation_count) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool ui_mesh_shader_config_is_valid(const ui_mesh_shader_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(0 == config_->vbo_size) {
        return false;
    }
    if(0 == config_->max_allocation_count) {
        return false;
    }
    if(!buffer_usage_is_valid(config_->buffer_usage)) {
        return false;
    }
    return true;
}

bool renderer_config_is_valid(const renderer_config_t* config_) {
    if(NULL == config_) {
        return false;
    }
    if(!line_mesh_shader_config_is_valid(&config_->line_mesh_shader_config)) {
        return false;
    }
    if(!lit_mesh_shader_config_is_valid(&config_->lit_mesh_shader_config)) {
        return false;
    }
    if(!point_mesh_shader_config_is_valid(&config_->point_mesh_shader_config)) {
        return false;
    }
    if(!ui_mesh_shader_config_is_valid(&config_->ui_mesh_shader_config)) {
        return false;
    }
    return true;
}
