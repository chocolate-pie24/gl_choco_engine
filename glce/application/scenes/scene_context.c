// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chocolate-pie24

#include "application/scenes/scene_context.h"

#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/platform_system/core/platform_system_types.h"

#include "application/scenes/core/scene_types.h"
#include "application/scenes/core/scene_err_utils.h"
#include "application/scenes/vtables/scene_vtables.h"
#include "application/scenes/scene_concretes/demos/sample_robot/sample_robot.h"
#include "application/scenes/scene_concretes/demos/stl_viewer/stl_viewer.h"

struct scene_context {
    void* scene;
    scene_list_t current_scene;
    const scene_vtable_t* vtable;
};

static const scene_vtable_t* scene_vtable_get(scene_list_t scene_);

scene_result_t scene_context_create(scene_context_t** out_scene_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    scene_context_t* tmp_scene_context = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_scene_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_scene_, ret, SCENE_BAD_OPERATION);

    ret_general_allocator = general_allocator_allocate(sizeof(scene_context_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_scene_context);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = scene_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("scene_context_create(%s) - general_allocator_allocate failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    tmp_scene_context->current_scene = SCENE_NONE;
    tmp_scene_context->scene = NULL;
    tmp_scene_context->vtable = NULL;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(tmp_scene_context)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("scene_context_create(%s) - Result validation failed for 'tmp_scene_context'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_scene_ = tmp_scene_context;
    tmp_scene_context = NULL;

    ret = SCENE_SUCCESS;

cleanup:
    if(SCENE_DATA_CORRUPTED == ret || SCENE_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_scene_context) {
        general_allocator_free((void**)&tmp_scene_context, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void scene_context_destroy(renderer_frontend_t* renderer_frontend_, scene_context_t** scene_) {
    if(NULL == renderer_frontend_ || NULL == scene_ || NULL == *scene_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(*scene_)) {
        ERROR_MESSAGE("scene_context_destroy(%s) - Precondition validation failed for '*scene_'.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
#endif

    if(NULL == (*scene_)->vtable) {
        general_allocator_free((void**)scene_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        return;
    }
    (*scene_)->vtable->scene_destroy(renderer_frontend_, &(*scene_)->scene);
    if(NULL != (*scene_)->scene) {
        ERROR_MESSAGE("scene_context_destroy(%s) - scene_destroy failed.", scene_result_to_str(SCENE_DATA_CORRUPTED));
        return;
    }
    general_allocator_free((void**)scene_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

scene_list_t scene_context_current_scene_get(const scene_context_t* scene_) {
    if(NULL == scene_) {
        return SCENE_NONE;
    }
    return scene_->current_scene;
}

// NOTE: scene_config_はsceneによっては不要であるためNULLを許可
scene_result_t scene_context_change_scene(scene_context_t* scene_, renderer_frontend_t* renderer_frontend_, const void* scene_config_, const char* asset_root_path_, int framebuffer_width_, int framebuffer_height_, scene_list_t next_scene_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    const scene_vtable_t* next_vtable = NULL;
    void* tmp_scene = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(scene_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, SCENE_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(scene_)) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("scene_context_change_scene(%s) - Precondition validation failed for 'scene_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif
    if(scene_->current_scene == next_scene_) {
        ret = SCENE_SUCCESS;
        goto cleanup;
    }

    // Preflight.
    if(SCENE_NONE != next_scene_) {
        if(0 >= framebuffer_width_ || 0 >= framebuffer_height_) {
            ret = SCENE_INVALID_ARGUMENT;
            ERROR_MESSAGE("scene_context_change_scene(%s) - Provided framebuffer_width_ or framebuffer_height_ is not valid.", scene_result_to_str(ret));
            goto cleanup;
        }
        next_vtable = scene_vtable_get(next_scene_);
        if(NULL == next_vtable) {
            ret = SCENE_INVALID_ARGUMENT;
            ERROR_MESSAGE("scene_context_change_scene(%s) - scene_vtable_get failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }

    // Destroy Current Scene.
    if(NULL != scene_->vtable) {
        scene_->vtable->scene_destroy(renderer_frontend_, &scene_->scene);
        if(NULL != scene_->scene) {
            ret = SCENE_DATA_CORRUPTED;
            ERROR_MESSAGE("scene_context_change_scene(%s) - scene_destroy failed.", scene_result_to_str(ret));
            goto cleanup;
        }
    }
    scene_->current_scene = SCENE_NONE;
    scene_->vtable = NULL;

    // Create Next Scene.
    if(SCENE_NONE == next_scene_) {
        ret = SCENE_SUCCESS;
        goto cleanup;
    }
    ret = next_vtable->scene_create(renderer_frontend_, scene_config_, asset_root_path_, framebuffer_width_, framebuffer_height_, &tmp_scene);
    if(SCENE_SUCCESS != ret) {
        ERROR_MESSAGE("scene_context_change_scene(%s) - scene_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }
    if(NULL == tmp_scene) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("scene_context_change_scene(%s) - scene_create failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    // Commit.
    scene_->scene = tmp_scene;
    scene_->current_scene = next_scene_;
    scene_->vtable = next_vtable;

    // Postcondition.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(scene_)) {
        ret = SCENE_DATA_CORRUPTED;
        ERROR_MESSAGE("scene_context_change_scene(%s) - Postcondition validation failed for 'scene_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    return SCENE_SUCCESS;

cleanup:
    return ret;
}

scene_result_t scene_context_update(scene_context_t* scene_, const platform_event_view_t* event_view_, float delta_time_, bool window_resized_, int framebuffer_width_, int framebuffer_height_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(scene_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(event_view_, ret, SCENE_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(scene_)) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("scene_context_update(%s) - Precondition validation failed for 'scene_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(SCENE_NONE == scene_->current_scene) {
        ret = SCENE_SUCCESS;
        goto cleanup;
    }
    ret = scene_->vtable->scene_update(scene_->scene, event_view_, delta_time_, window_resized_, framebuffer_width_, framebuffer_height_);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("scene_context_update(%s) - scene_update failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

scene_result_t scene_context_draw(scene_context_t* scene_, renderer_frontend_t* renderer_frontend_) {
    scene_result_t ret = SCENE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(scene_, ret, SCENE_INVALID_ARGUMENT);
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, SCENE_INVALID_ARGUMENT);
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!scene_context_is_valid(scene_)) {
        ret = SCENE_INVALID_ARGUMENT;
        ERROR_MESSAGE("scene_context_draw(%s) - Precondition validation failed for 'scene_'.", scene_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(SCENE_NONE == scene_->current_scene) {
        ret = SCENE_SUCCESS;
        goto cleanup;
    }
    ret = scene_->vtable->scene_draw(scene_->scene, renderer_frontend_);
    if(SCENE_SUCCESS != ret) {
        if(SCENE_INVALID_ARGUMENT == ret) {
            ret = SCENE_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("scene_context_draw(%s) - scene_draw failed.", scene_result_to_str(ret));
        goto cleanup;
    }

    ret = SCENE_SUCCESS;

cleanup:
    return ret;
}

bool scene_context_is_valid(const scene_context_t* scene_) {
    if(NULL == scene_) {
        return false;
    }
    if(SCENE_SAMPLE_ROBOT != scene_->current_scene &&
       SCENE_STL_VIEWER != scene_->current_scene &&
       SCENE_NONE != scene_->current_scene) {
        return false;
    }
    if(SCENE_NONE == scene_->current_scene) {
        return (NULL == scene_->scene && NULL == scene_->vtable);
    }
    if(SCENE_SAMPLE_ROBOT == scene_->current_scene && (scene_->vtable != sample_robot_vtable_get() || NULL == scene_->scene)) {
        return false;
    }
    if(SCENE_STL_VIEWER == scene_->current_scene && (scene_->vtable != stl_viewer_vtable_get() || NULL == scene_->scene)) {
        return false;
    }
    if(!scene_->vtable->scene_is_valid(scene_->scene)) {
        return false;
    }
    return true;
}

static const scene_vtable_t* scene_vtable_get(scene_list_t scene_) {
    switch(scene_) {
    case SCENE_NONE:
        return NULL;
    case SCENE_STL_VIEWER:
        return stl_viewer_vtable_get();
    case SCENE_SAMPLE_ROBOT:
        return sample_robot_vtable_get();
    default:
        return NULL;
    }
}
