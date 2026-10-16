// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/model_transform_tree.h"

#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_err_utils.h"

static model_transform_node_t* node_find_by_id(model_transform_node_t* node_, uint16_t coordinate_id_);

render_model_result_t model_transform_tree_initialize(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    model_transform_node_t* tmp_root_node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_initialize", "transform_tree_")
    if(NULL != transform_tree_->root) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_initialize(%s) - Provided transform_tree_ is already initialized.", render_model_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(model_transform_node_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_root_node);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("model_transform_tree_initialize(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }
    tmp_root_node->child = NULL;
    tmp_root_node->next = NULL;
    tmp_root_node->parent = NULL;

    tmp_root_node->coordinate_id = coordinate_id_;
    tmp_root_node->rotation = vec3f_initialize(0.0f, 0.0f, 0.0f);
    tmp_root_node->translate = vec3f_initialize(0.0f, 0.0f, 0.0f);

    transform_tree_->root = tmp_root_node;
    tmp_root_node = NULL;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    if(RENDER_MODEL_DATA_CORRUPTED != ret) {
        if(NULL != tmp_root_node) {
            general_allocator_free((void**)&tmp_root_node, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
    }
    return ret;
}

void model_transform_tree_deinitialize(model_transform_tree_t* transform_tree_) {
    model_transform_node_t* node = NULL;
    model_transform_node_t* parent = NULL;
    model_transform_node_t* next = NULL;

    if(NULL == transform_tree_) {
        return;
    }
    node = transform_tree_->root;
    while(NULL != node) {
        if(NULL != node->child) {
            node = node->child;
            continue;
        }
        parent = node->parent;
        next = node->next;

        general_allocator_free((void**)&node, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        if(NULL == parent) {
            transform_tree_->root = NULL;
            return;
        }
        parent->child = next;
        if(NULL != next) {
            node = next;
        } else {
            node = parent;
        }
    }
}

render_model_result_t model_transform_tree_add_child(model_transform_tree_t* transform_tree_, uint16_t parent_coordinate_id_, uint16_t coordinate_id_, vec3f_t translate_, vec3f_t rotation_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    model_transform_node_t* parent = NULL;
    model_transform_node_t* child = NULL;
    model_transform_node_t* next = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_add_child", "transform_tree_")
    if(NULL == transform_tree_->root) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - Provided transform_tree_ is not initialized.", render_model_result_to_str(ret));
        goto cleanup;
    }
    if(NULL != node_find_by_id(transform_tree_->root, coordinate_id_)) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - Provided coordinate_id_ is already registered.", render_model_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    parent = node_find_by_id(transform_tree_->root, parent_coordinate_id_);
    if(NULL == parent) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - Provided parent_coordinate_id_ not found.", render_model_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(sizeof(model_transform_node_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&child);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }
    child->child = NULL;
    child->next = NULL;
    child->parent = parent;
    child->rotation = rotation_;
    child->translate = translate_;
    child->coordinate_id = coordinate_id_;

    // Commit.
    if(NULL == parent->child) {
        parent->child = child;
    } else {
        next = parent->child;
        while(NULL != next->next) {
            next = next->next;
        }
        next->next = child;
    }
    child = NULL;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    if(RENDER_MODEL_DATA_CORRUPTED != ret) {
        if(NULL != child) {
            general_allocator_free((void**)&child, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
    }
    return ret;
}

render_model_result_t model_transform_tree_rotation_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t rotation_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    model_transform_node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_rotation_set", "transform_tree_")
    if(NULL == transform_tree_->root) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_rotation_set(%s) - Provided transform_tree_ is not initialized.", render_model_result_to_str(ret));
        goto cleanup;
    }
    node = node_find_by_id(transform_tree_->root, coordinate_id_);
    if(NULL == node) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_transform_tree_rotation_set(%s) - Provided coordinate_id_ not found.", render_model_result_to_str(ret));
        goto cleanup;
    }

    node->rotation = rotation_;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

render_model_result_t model_transform_tree_translate_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t translate_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    model_transform_node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_translate_set", "transform_tree_")
    if(NULL == transform_tree_->root) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_translate_set(%s) - Provided transform_tree_ is not initialized.", render_model_result_to_str(ret));
        goto cleanup;
    }
    node = node_find_by_id(transform_tree_->root, coordinate_id_);
    if(NULL == node) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_transform_tree_translate_set(%s) - Provided coordinate_id_ not found.", render_model_result_to_str(ret));
        goto cleanup;
    }

    node->translate = translate_;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

render_model_result_t model_transform_tree_world_matrix_get(const model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, mat4x4f_t* out_matrix_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    model_transform_node_t* node = NULL;
    mat4x4f_t mat = { 0 };
    mat4x4f_t rotation = { 0 };
    mat4x4f_t translation = { 0 };
    mat4x4f_t local = { 0 };
    mat4x4f_t tmp = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_world_matrix_get", "transform_tree_")
    IF_ARG_NULL_GOTO_CLEANUP(out_matrix_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_world_matrix_get", "out_matrix_")
    if(NULL == transform_tree_->root) {
        ret = RENDER_MODEL_BAD_OPERATION;
        ERROR_MESSAGE("model_transform_tree_world_matrix_get(%s) - Provided transform_tree_ is not initialized.", render_model_result_to_str(ret));
        goto cleanup;
    }
    node = node_find_by_id(transform_tree_->root, coordinate_id_);
    if(NULL == node) {
        ret = RENDER_MODEL_INVALID_ARGUMENT;
        ERROR_MESSAGE("model_transform_tree_world_matrix_get(%s) - Provided coordinate_id_ not found.", render_model_result_to_str(ret));
        goto cleanup;
    }

    mat4f_identity(&mat);
    while(NULL != node) {
        mat4f_rot_xyz(CHOCO_DEG_TO_RAD(node->rotation.elem[0]), CHOCO_DEG_TO_RAD(node->rotation.elem[1]), CHOCO_DEG_TO_RAD(node->rotation.elem[2]), &rotation);
        mat4f_translation(node->translate, &translation);
        mat4f_mul(&translation, &rotation, &local);
        mat4f_mul(&local, &mat, &tmp);
        mat = tmp;
        node = node->parent;
    }

    *out_matrix_ = mat;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    return ret;
}

static model_transform_node_t* node_find_by_id(model_transform_node_t* node_, uint16_t coordinate_id_) {
    if(NULL == node_) {
        return NULL;
    } else if(node_->coordinate_id == coordinate_id_) {
        return node_;
    }

    model_transform_node_t* child = node_->child;
    while(NULL != child) {
        model_transform_node_t* found = node_find_by_id(child, coordinate_id_);
        if(NULL != found) {
            return found;
        }
        child = child->next;
    }
    return NULL;
}
