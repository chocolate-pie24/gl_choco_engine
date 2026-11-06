// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file untextured_mesh_shader.c
 * @author chocolate-pie24
 * @brief 光源・法線・材質色などを使って陰影付きMeshを描画するUntextured Mesh Shader Resourceの生成・破棄、VAO管理、Uniform設定APIの実装
 *
 * @date 2026-06-03
 *
 */
#include "engine/systems/renderer/resources/shaders/untextured_mesh_shader.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/resources/materials/material_types.h"

#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_shader.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vao.h"
#include "engine/systems/renderer/renderer_backend/renderer_backend_vbo.h"

#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/shaders/core/shader_err_utils.h"
#include "engine/systems/renderer/resources/shaders/core/shader_program_builder.h"

typedef struct {
    int32_t model_matrix_location;          /**< モデル行列のユニフォーム変数Location */
    int32_t view_matrix_location;           /**< ビュー行列のユニフォーム変数Location */
    int32_t projection_matrix_location;     /**< プロジェクション行列のユニフォーム変数Location */

    int32_t material_ambient_location;
    int32_t material_diffuse_location;
    int32_t material_specular_location;
    int32_t material_shininess_location;

    int32_t directional_light_direction_location;
    int32_t directional_light_color_location;
    int32_t directional_light_intensity_location;

    int32_t ambient_light_color_location;
    int32_t ambient_light_intensity_location;
} uniform_location_t;

struct untextured_mesh_shader {
    uniform_location_t uniform_location;
    renderer_backend_shader_t* shader;      /**< シェーダープログラムハンドルインスタンスへのポインタ */
    renderer_backend_vao_t* vao;            /** シェーダーVAO */
};

static shader_result_t uniform_location_initialize(const renderer_backend_context_t* backend_context_, renderer_backend_shader_t* backend_shader_, uniform_location_t* out_uniform_location_);
static shader_result_t attribute_initialize(renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vao_, const renderer_backend_vbo_t* vbo_);

shader_result_t untextured_mesh_shader_create(const char* vertex_shader_fullpath_, const char* fragment_shader_fullpath_, renderer_backend_context_t* backend_context_, const renderer_backend_vbo_t* vbo_, untextured_mesh_shader_t** out_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    untextured_mesh_shader_t* tmp_untextured_mesh_shader = NULL;
    renderer_backend_shader_t* tmp_backend_shader = NULL;
    renderer_backend_vao_t* tmp_backend_vao = NULL;

    uniform_location_t tmp_uniform_location = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(vertex_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_create", "vertex_shader_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(fragment_shader_fullpath_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_create", "fragment_shader_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_create", "vbo_")
    IF_ARG_NULL_GOTO_CLEANUP(out_shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_create", "out_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_, ret, SHADER_BAD_OPERATION, shader_result_to_str(SHADER_BAD_OPERATION), "untextured_mesh_shader_create", "*out_shader_")
    if('\0' == vertex_shader_fullpath_[0]) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - Provided vertex_shader_fullpath_ is not valid.", shader_result_to_str(ret));
        goto cleanup;
    }
    if('\0' == fragment_shader_fullpath_[0]) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - Provided fragment_shader_fullpath_ is not valid.", shader_result_to_str(ret));
        goto cleanup;
    }

    // Resource Allocation.
    ret_general_allocator = general_allocator_allocate(sizeof(untextured_mesh_shader_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_untextured_mesh_shader);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = shader_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - general_allocator_allocate failed.", shader_result_to_str(ret));
        goto cleanup;
    }
    ret_renderer_backend = renderer_backend_vao_create(backend_context_, &tmp_backend_vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - renderer_backend_vao_create failed.", shader_result_to_str(ret));
        goto cleanup;
    }
    ret = shader_program_builder_create_from_files(backend_context_, vertex_shader_fullpath_, fragment_shader_fullpath_, &tmp_backend_shader);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - shader_program_builder_create_from_files failed.", shader_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret = uniform_location_initialize(backend_context_, tmp_backend_shader, &tmp_uniform_location);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - uniform_location_initialize failed.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = attribute_initialize(backend_context_, tmp_backend_vao, vbo_);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - attribute_initialize failed.", shader_result_to_str(ret));
        goto cleanup;
    }

    tmp_untextured_mesh_shader->uniform_location = tmp_uniform_location;
    tmp_untextured_mesh_shader->shader = tmp_backend_shader;
    tmp_untextured_mesh_shader->vao = tmp_backend_vao;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, tmp_untextured_mesh_shader)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_create(%s) - Result validation failed for 'tmp_untextured_mesh_shader'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_shader_ = tmp_untextured_mesh_shader;
    tmp_untextured_mesh_shader = NULL;
    tmp_backend_shader = NULL;
    tmp_backend_vao = NULL;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_DATA_CORRUPTED == ret || SHADER_SUCCESS == ret) {
        return ret;
    }
    if(NULL != backend_context_ && NULL != tmp_backend_shader) {
        renderer_backend_shader_destroy(backend_context_, &tmp_backend_shader);
    }
    if(NULL != backend_context_ && NULL != tmp_backend_vao) {
        renderer_backend_vao_destroy(backend_context_, &tmp_backend_vao);
    }
    if(NULL != tmp_untextured_mesh_shader) {
        general_allocator_free((void**)&tmp_untextured_mesh_shader, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void untextured_mesh_shader_destroy(renderer_backend_context_t* backend_context_, untextured_mesh_shader_t** shader_) {
    if(NULL == shader_ || NULL == *shader_) {
        return;
    }
    if(NULL == backend_context_) {
        ERROR_MESSAGE("untextured_mesh_shader_destroy - backend_context_ is NULL.");
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, *shader_)) {
        ERROR_MESSAGE("untextured_mesh_shader_destroy(%s) - Precondition validation failed for '*shader_'.", shader_result_to_str(SHADER_DATA_CORRUPTED));
        return;
    }
#endif

    renderer_backend_vao_destroy(backend_context_, &(*shader_)->vao);
    renderer_backend_shader_destroy(backend_context_, &(*shader_)->shader);
    general_allocator_free((void**)shader_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

shader_result_t untextured_mesh_shader_vao_bind(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_vao_bind", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_vao_bind", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_vao_bind(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_vao_bind(backend_context_, shader_->vao);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_vao_bind(%s) - Failed to bind vertex array.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_use(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_use", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_use(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_use(backend_context_, shader_->shader);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_use(%s) - Failed to switch program for untextured_mesh_shader.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_model_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* model_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_model_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_model_matrix_set", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_model_matrix_set", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_model_matrix_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, shader_->uniform_location.model_matrix_location, should_transpose_, model_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_model_matrix_set(%s) - Failed to set model matrix.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_view_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* view_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_view_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_view_matrix_set", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_view_matrix_set", "view_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_view_matrix_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, shader_->uniform_location.view_matrix_location, should_transpose_, view_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_view_matrix_set(%s) - Failed to set view matrix.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_projection_matrix_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const mat4x4f_t* projection_matrix_, bool should_transpose_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_projection_matrix_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_projection_matrix_set", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_projection_matrix_set", "projection_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_projection_matrix_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_mat4f_uniform_set(backend_context_, shader_->uniform_location.projection_matrix_location, should_transpose_, projection_matrix_->elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_projection_matrix_set(%s) - Failed to set projection matrix.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_directional_light_direction_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t direction_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_directional_light_direction_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_directional_light_direction_set", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_directional_light_direction_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.directional_light_direction_location, direction_.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_directional_light_direction_set(%s) - Failed to set light direction.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_directional_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_directional_light_color_intensity_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_directional_light_color_intensity_set", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_directional_light_direction_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.directional_light_color_location, color_.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_directional_light_color_intensity_set(%s) - Failed to set light color.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_float_uniform_set(backend_context_, shader_->uniform_location.directional_light_intensity_location, intensity_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_directional_light_color_intensity_set(%s) - Failed to set light intensity.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_ambient_light_color_intensity_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, vec3f_t color_, float intensity_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_ambient_light_color_intensity_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_ambient_light_color_intensity_set", "shader_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_ambient_light_color_intensity_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.ambient_light_color_location, color_.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_ambient_light_color_intensity_set(%s) - Failed to set light color.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_float_uniform_set(backend_context_, shader_->uniform_location.ambient_light_intensity_location, intensity_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_ambient_light_color_intensity_set(%s) - Failed to set light intensity.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

shader_result_t untextured_mesh_shader_material_set(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_, const untextured_mesh_material_t* material_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_material_set", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_material_set", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(material_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "untextured_mesh_shader_material_set", "material_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_mesh_shader_is_valid(backend_context_, shader_)) {
        ret = SHADER_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Precondition validation failed for 'shader_'.", shader_result_to_str(ret));
        goto cleanup;
    }
    if(!untextured_mesh_material_is_valid(material_)) {
        ret = SHADER_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Provided material_ is not valid.", shader_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.material_ambient_location, material_->ambient.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Failed to set ambient.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.material_diffuse_location, material_->diffuse.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Failed to set diffuse.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_vec3f_uniform_set(backend_context_, shader_->uniform_location.material_specular_location, material_->specular.elem);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Failed to set specular.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_float_uniform_set(backend_context_, shader_->uniform_location.material_shininess_location, material_->shininess);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("untextured_mesh_shader_material_set(%s) - Failed to set shininess.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

bool untextured_mesh_shader_is_valid(const renderer_backend_context_t* backend_context_, const untextured_mesh_shader_t* shader_) {
    if(NULL == backend_context_ || NULL == shader_) {
        return false;
    }
    if(NULL == shader_->shader || NULL == shader_->vao) {
        return false;
    }

    if(0 > shader_->uniform_location.model_matrix_location) {
        return false;
    }
    if(0 > shader_->uniform_location.view_matrix_location) {
        return false;
    }
    if(0 > shader_->uniform_location.projection_matrix_location) {
        return false;
    }
    if(0 > shader_->uniform_location.material_ambient_location) {
        return false;
    }
    if(0 > shader_->uniform_location.material_diffuse_location) {
        return false;
    }
    if(0 > shader_->uniform_location.material_specular_location) {
        return false;
    }
    if(0 > shader_->uniform_location.material_shininess_location) {
        return false;
    }

    if(0 > shader_->uniform_location.directional_light_direction_location) {
        return false;
    }
    if(0 > shader_->uniform_location.directional_light_color_location) {
        return false;
    }
    if(0 > shader_->uniform_location.directional_light_intensity_location) {
        return false;
    }

    if(0 > shader_->uniform_location.ambient_light_color_location) {
        return false;
    }
    if(0 > shader_->uniform_location.ambient_light_intensity_location) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(shader_->shader)) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(shader_->vao)) {
        return false;
    }

    // backend_context_のvalidationはrenderer_backend_xxx_is_validで実行されるため不要
    if(!renderer_backend_shader_is_valid(backend_context_, shader_->shader)) {
        return false;
    }
    if(!renderer_backend_vao_is_valid(backend_context_, shader_->vao)) {
        return false;
    }

    return true;
}

static shader_result_t uniform_location_initialize(const renderer_backend_context_t* backend_context_, renderer_backend_shader_t* backend_shader_, uniform_location_t* out_uniform_location_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    int32_t tmp_model_matrix_location = 0;
    int32_t tmp_view_matrix_location = 0;
    int32_t tmp_projection_matrix_location = 0;

    int32_t tmp_material_ambient_location = 0;
    int32_t tmp_material_diffuse_location = 0;
    int32_t tmp_material_specular_location = 0;
    int32_t tmp_material_shininess_location = 0;

    int32_t tmp_directional_light_direction_location = 0;
    int32_t tmp_directional_light_color_location = 0;
    int32_t tmp_directional_light_intensity_location = 0;

    int32_t tmp_ambient_light_color_location = 0;
    int32_t tmp_ambient_light_intensity_location = 0;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "uniform_location_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_shader_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "uniform_location_initialize", "backend_shader_")
    IF_ARG_NULL_GOTO_CLEANUP(out_uniform_location_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "uniform_location_initialize", "out_uniform_location_")

    // uniform location
    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_model_matrix", &tmp_model_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get model matrix location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_view_matrix", &tmp_view_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get view matrix location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_projection_matrix", &tmp_projection_matrix_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get projection matrix location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_material.ambient", &tmp_material_ambient_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get ambient location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_material.diffuse", &tmp_material_diffuse_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get diffuse location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_material.specular", &tmp_material_specular_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get specular location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_material.shininess", &tmp_material_shininess_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get shininess location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_directional_light_direction", &tmp_directional_light_direction_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get directional light direction location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_directional_light_color", &tmp_directional_light_color_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get directional light color location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_directional_light_intensity", &tmp_directional_light_intensity_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get directional light intensity location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_ambient_light_color", &tmp_ambient_light_color_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get ambient light color location.", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_uniform_location_get(backend_context_, backend_shader_, "g_ambient_light_intensity", &tmp_ambient_light_intensity_location);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("uniform_location_initialize(%s) - Failed to get ambient light intensity location.", shader_result_to_str(ret));
        goto cleanup;
    }

    out_uniform_location_->model_matrix_location = tmp_model_matrix_location;
    out_uniform_location_->view_matrix_location = tmp_view_matrix_location;
    out_uniform_location_->projection_matrix_location = tmp_projection_matrix_location;

    out_uniform_location_->material_ambient_location = tmp_material_ambient_location;
    out_uniform_location_->material_diffuse_location = tmp_material_diffuse_location;
    out_uniform_location_->material_shininess_location = tmp_material_shininess_location;
    out_uniform_location_->material_specular_location = tmp_material_specular_location;

    out_uniform_location_->directional_light_direction_location = tmp_directional_light_direction_location;
    out_uniform_location_->directional_light_color_location = tmp_directional_light_color_location;
    out_uniform_location_->directional_light_intensity_location = tmp_directional_light_intensity_location;

    out_uniform_location_->ambient_light_color_location = tmp_ambient_light_color_location;
    out_uniform_location_->ambient_light_intensity_location = tmp_ambient_light_intensity_location;

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}

static shader_result_t attribute_initialize(renderer_backend_context_t* backend_context_, const renderer_backend_vao_t* vao_, const renderer_backend_vbo_t* vbo_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    shader_result_t ret_cleanup = SHADER_INVALID_ARGUMENT;
    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    bool vao_bound = false;
    bool vbo_bound = false;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "attribute_initialize", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(vao_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "attribute_initialize", "vao_")
    IF_ARG_NULL_GOTO_CLEANUP(vbo_, ret, SHADER_INVALID_ARGUMENT, shader_result_to_str(SHADER_INVALID_ARGUMENT), "attribute_initialize", "vbo_")

    ret_renderer_backend = renderer_backend_vao_bind(backend_context_, vao_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to bind vertex array.", shader_result_to_str(ret));
        goto cleanup;
    }
    vao_bound = true;

    ret_renderer_backend = renderer_backend_vbo_bind(backend_context_, vbo_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to bind vertex buffer.",shader_result_to_str(ret));
        goto cleanup;
    }
    vbo_bound = true;

    // attribute
    // position: vec3f_t 12byte
    // normal: vec4i8_t 4byte(x, y, z, padding)
    ret_renderer_backend = renderer_backend_vao_attribute_set(backend_context_, 0, 3, RENDERER_TYPE_FLOAT, false, sizeof(point_normal_vertex_t), offsetof(point_normal_vertex_t, position));  // 座標情報(layout = 0)
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to set vertex array attribute(position).", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vao_attribute_set(backend_context_, 1, 3, RENDERER_TYPE_BYTE, true, sizeof(point_normal_vertex_t), offsetof(point_normal_vertex_t, normal));    // 法線情報(layout = 1)
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to set vertex array attribute(normal).", shader_result_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to unbind vertex buffer.",shader_result_to_str(ret));
        goto cleanup;
    }
    vbo_bound = false;

    ret_renderer_backend = renderer_backend_vao_unbind(backend_context_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_result_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("attribute_initialize(%s) - Failed to unbind vertex array.", shader_result_to_str(ret));
        goto cleanup;
    }
    vao_bound = false;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_DATA_CORRUPTED == ret || SHADER_SUCCESS == ret) {
        return ret;
    }
    if(vbo_bound) {
        ret_renderer_backend = renderer_backend_vbo_unbind(backend_context_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret_cleanup = shader_result_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("attribute_initialize failed. %s", shader_result_to_str(ret_cleanup));
            ret = SHADER_DATA_CORRUPTED;
            return ret;
        }
    }
    if(vao_bound) {
        ret_renderer_backend = renderer_backend_vao_unbind(backend_context_);
        if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
            ret_cleanup = shader_result_convert_renderer_backend(ret_renderer_backend);
            ERROR_MESSAGE("attribute_initialize failed. %s", shader_result_to_str(ret_cleanup));
            ret = SHADER_DATA_CORRUPTED;
            return ret;
        }
    }
    return ret;
}
