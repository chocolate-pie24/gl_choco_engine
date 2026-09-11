// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/render_resources/lit_mesh_render_resource.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h>
#include <stdint.h>

#include <GL/glew.h>    // TODO: remove this!! glfwSwapBuffersをrendererに移したら削除

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"

#include "engine/core/memory/linear_allocator.h"
#include "engine/core/geometry_primitive/aabb_3d.h"
#include "engine/core/geometry_primitive/vertex.h"

#include "engine/io_utils/fs_path.h"

#include "engine/resource/geometry/conversion/geometry_conversion.h"

#include "engine/systems/renderer/core/renderer_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resources/shaders/lit_mesh_shader.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/geometries/lit_mesh_geometry_registry.h"
#include "engine/systems/renderer/resource_pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resource_pipelines/geometries/lit_mesh_geometry_pipeline.h"

#include "engine/systems/renderer/render_resources/core/render_resource_types.h"
#include "engine/systems/renderer/render_resources/core/render_resource_err_utils.h"

struct lit_mesh_render_resource {
    lit_mesh_shader_t* shader;
    lit_mesh_geometry_registry_t* geometry_registry;
};

static render_resource_result_t shader_create(const lit_mesh_shader_config_t* lit_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, lit_mesh_shader_t** out_lit_mesh_shader_);
static bool is_valid_shallow(const lit_mesh_render_resource_t* render_resource_);

render_resource_result_t lit_mesh_render_resource_initialize(const lit_mesh_shader_config_t* shader_config_, size_t max_geometry_count_, renderer_backend_context_t* renderer_backend_context_, linear_alloc_t* allocator_, const char* executable_directory_, const char* shader_dir_, lit_mesh_render_resource_t** out_render_resource_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    lit_mesh_render_resource_t* tmp_render_resource = NULL;
    lit_mesh_shader_t* tmp_shader = NULL;
    lit_mesh_geometry_registry_t* tmp_registry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(shader_config_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "shader_config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_initialize", "out_render_resource_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_resource_, ret, RENDER_RESOURCE_BAD_OPERATION, render_resource_rslt_to_str(RENDER_RESOURCE_BAD_OPERATION), "lit_mesh_render_resource_initialize", "*out_render_resource_")
    if(0 == max_geometry_count_) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - Provided max_geometry_count_ is not valid.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == executable_directory_[0]) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - Provided executable_directory_ is not valid.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == shader_dir_[0]) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - Provided shader_dir_ is not valid.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(lit_mesh_render_resource_t), alignof(lit_mesh_render_resource_t), (void**)&tmp_render_resource);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = render_resource_rslt_convert_linear_allocator(ret_linear_alloc);
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - Failed to allocate lit_mesh_render_resource_t instance.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_render_resource, 0, sizeof(lit_mesh_render_resource_t));

    ret = shader_create(shader_config_, renderer_backend_context_, executable_directory_, shader_dir_, &tmp_shader);
    if(RENDER_RESOURCE_SUCCESS != ret) {
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - shader_create failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_resource_registry = lit_mesh_geometry_registry_initialize(max_geometry_count_, allocator_, &tmp_registry);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = render_resource_rslt_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - lit_mesh_geometry_registry_initialize failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    tmp_render_resource->shader = tmp_shader;
    tmp_render_resource->geometry_registry = tmp_registry;

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(tmp_render_resource)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_initialize(%s) - Postcondition validation failed for 'tmp_render_resource'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_render_resource_ = tmp_render_resource;
    tmp_render_resource = NULL;
    tmp_shader = NULL;
    tmp_registry = NULL;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    if(RENDER_RESOURCE_DATA_CORRUPTED != ret) {
        if(NULL != tmp_registry && NULL != tmp_shader) {
            lit_mesh_geometry_registry_deinitialize(tmp_registry, tmp_shader);
        }
        if(NULL != tmp_shader) {
            lit_mesh_shader_destroy(&tmp_shader);
        }
    }
    return ret;
}

void lit_mesh_render_resource_deinitialize(lit_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(render_resource_)) {
        ERROR_MESSAGE("lit_mesh_render_resource_deinitialize(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(RENDER_RESOURCE_DATA_CORRUPTED));
        return;
    }
#endif

    lit_mesh_geometry_registry_deinitialize(render_resource_->geometry_registry, render_resource_->shader);
    lit_mesh_shader_destroy(&render_resource_->shader);
}

render_resource_result_t lit_mesh_render_resource_geometry_import_from_file(lit_mesh_render_resource_t* render_resource_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    uint16_t tmp_geometry_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_geometry_import_from_file", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_geometry_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_geometry_import_from_file", "resource_fullpath_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_geometry_import_from_file", "out_geometry_id_")
    if('\0' == resource_name_[0]) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_import_from_file(%s) - Provided resource_name_ is not valid.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == resource_fullpath_[0]) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_import_from_file(%s) - Provided resource_fullpath_ is not valid.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_import_from_file(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = lit_mesh_geometry_pipeline_import_from_file(render_resource_->shader, render_resource_->geometry_registry, resource_name_, resource_fullpath_, &tmp_geometry_id);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_rslt_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_import_from_file(%s) - lit_mesh_geometry_pipeline_import_from_file failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_import_from_file(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    *out_geometry_id_ = tmp_geometry_id;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t lit_mesh_render_resource_geometry_release(lit_mesh_render_resource_t* render_resource_, uint16_t geometry_id_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_pipeline_result_t ret_resource_pipeline = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_geometry_release", "render_resource_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_release(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_resource_pipeline = lit_mesh_geometry_pipeline_release(render_resource_->shader, render_resource_->geometry_registry, geometry_id_);
    if(RESOURCE_PIPELINE_SUCCESS != ret_resource_pipeline) {
        ret = render_resource_rslt_convert_resource_pipeline(ret_resource_pipeline);
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_release(%s) - lit_mesh_geometry_pipeline_release failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_geometry_release(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t lit_mesh_render_resource_view_matrix_set(lit_mesh_render_resource_t* render_resource_, const mat4x4f_t* view_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_view_matrix_set", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(view_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_view_matrix_set", "view_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_view_matrix_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = lit_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_view_matrix_set(%s) - lit_mesh_shader_use failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_view_matrix_set(render_resource_->shader, view_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_view_matrix_set(%s) - lit_mesh_shader_view_matrix_set failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_view_matrix_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t lit_mesh_render_resource_projection_matrix_set(lit_mesh_render_resource_t* render_resource_, const mat4x4f_t* projection_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_projection_matrix_set", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(projection_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_projection_matrix_set", "projection_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_projection_matrix_set(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = lit_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_projection_matrix_set(%s) - lit_mesh_shader_use failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_projection_matrix_set(render_resource_->shader, projection_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_projection_matrix_set(%s) - lit_mesh_shader_projection_matrix_set failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!lit_mesh_render_resource_is_valid(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_projection_matrix_set(%s) - Postcondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

// NOTE: renderer_backend_vao_unbindを行う場合renderer_backend_contextが必要となる、ただ、実行しなくても良いので当面は実行しない
render_resource_result_t lit_mesh_render_resource_draw(lit_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, const mat4x4f_t* model_matrix_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;

    const draw_range_t* draw_range = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_draw", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_draw", "model_matrix_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_draw(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    ret_shader = lit_mesh_shader_use(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_draw(%s) - lit_mesh_shader_use failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_model_matrix_set(render_resource_->shader, model_matrix_, true);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_draw(%s) - lit_mesh_shader_model_matrix_set failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_vao_bind(render_resource_->shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("lit_mesh_render_resource_draw(%s) - lit_mesh_shader_vao_bind failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    draw_range = lit_mesh_geometry_registry_draw_range_get(render_resource_->geometry_registry, geometry_id_);
    if(NULL == draw_range) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_draw(%s) - lit_mesh_geometry_registry_draw_range_get failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    // TODO: renderer_backendにgl系APIを追加しOpenGL依存を消す
    glDrawArrays(GL_TRIANGLES, (GLint)draw_range->first_vertex_count, (GLint)draw_range->vertex_count);

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

render_resource_result_t lit_mesh_render_resource_convert_to_aabb_3d(lit_mesh_render_resource_t* render_resource_, uint16_t geometry_id_, aabb_3d_t* out_aabb_3d_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    resource_result_t ret_resource = RESOURCE_INVALID_ARGUMENT;

    aabb_3d_t tmp_aabb_3d = { 0 };
    const lit_mesh_geometry_t* tmp_geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(render_resource_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_convert_to_aabb_3d", "render_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_aabb_3d_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "lit_mesh_render_resource_convert_to_aabb_3d", "out_aabb_3d_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!is_valid_shallow(render_resource_)) {
        ret = RENDER_RESOURCE_DATA_CORRUPTED;
        ERROR_MESSAGE("lit_mesh_render_resource_convert_to_aabb_3d(%s) - Precondition validation failed for 'render_resource_'.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }
#endif

    tmp_geometry = lit_mesh_geometry_registry_geometry_get(render_resource_->geometry_registry, geometry_id_);
    if(NULL == tmp_geometry) {
        ret = RENDER_RESOURCE_INVALID_ARGUMENT;
        ERROR_MESSAGE("lit_mesh_render_resource_convert_to_aabb_3d(%s) - lit_mesh_geometry_registry_geometry_get failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_resource = geometry_conversion_lit_mesh_geometry_to_aabb_3d(tmp_geometry, &tmp_aabb_3d);
    if(RESOURCE_SUCCESS != ret_resource) {
        ret = render_resource_rslt_convert_resource(ret_resource);
        ERROR_MESSAGE("lit_mesh_render_resource_convert_to_aabb_3d(%s) - geometry_conversion_lit_mesh_geometry_to_aabb_3d failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    // aabb_3d_tのcanonical validator追加後にtmp_aabb_3dのvalidationを実行する

    *out_aabb_3d_ = tmp_aabb_3d;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    return ret;
}

bool lit_mesh_render_resource_is_valid(const lit_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return false;
    }
    if(!is_valid_shallow(render_resource_)) {
        return false;
    }
    if(!lit_mesh_geometry_registry_is_valid(render_resource_->geometry_registry)) {
        return false;
    }

    // TODO: lit_mesh_shader_is_valid追加後にrender_resource_->shaderのvalidationを追加する
    return true;
}

static render_resource_result_t shader_create(const lit_mesh_shader_config_t* lit_mesh_shader_config_, renderer_backend_context_t* renderer_backend_context_, const char* executable_directory_, const char* shader_dir_, lit_mesh_shader_t** out_lit_mesh_shader_) {
    render_resource_result_t ret = RENDER_RESOURCE_INVALID_ARGUMENT;

    shader_result_t ret_shader = SHADER_INVALID_ARGUMENT;
    fs_path_result_t ret_fs_path = FS_PATH_INVALID_ARGUMENT;

    fs_path_t* vertex_shader_path = NULL;
    fs_path_t* fragment_shader_path = NULL;

    lit_mesh_shader_t* tmp_lit_mesh_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(lit_mesh_shader_config_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "lit_mesh_shader_config_")
    IF_ARG_NULL_GOTO_CLEANUP(renderer_backend_context_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "renderer_backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(executable_directory_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "executable_directory_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_dir_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "shader_dir_")
    IF_ARG_NULL_GOTO_CLEANUP(out_lit_mesh_shader_, ret, RENDER_RESOURCE_INVALID_ARGUMENT, render_resource_rslt_to_str(RENDER_RESOURCE_INVALID_ARGUMENT), "shader_create", "out_lit_mesh_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_lit_mesh_shader_, ret, RENDER_RESOURCE_BAD_OPERATION, render_resource_rslt_to_str(RENDER_RESOURCE_BAD_OPERATION), "shader_create", "*out_lit_mesh_shader_")

    ret_fs_path = fs_path_create(&vertex_shader_path, executable_directory_, shader_dir_, "lit_mesh_shader", "vert");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = render_resource_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("shader_create(%s) - fs_path_create failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_path = fs_path_create(&fragment_shader_path, executable_directory_, shader_dir_, "lit_mesh_shader", "frag");
    if(FS_PATH_SUCCESS != ret_fs_path) {
        ret = render_resource_rslt_convert_fs_path(ret_fs_path);
        ERROR_MESSAGE("shader_create(%s) - fs_path_create failed.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    ret_shader = lit_mesh_shader_create(renderer_backend_context_, fs_path_fullpath_get(vertex_shader_path), fs_path_fullpath_get(fragment_shader_path), lit_mesh_shader_config_, &tmp_lit_mesh_shader);
    if(SHADER_SUCCESS != ret_shader) {
        ret = render_resource_rslt_convert_shader(ret_shader);
        ERROR_MESSAGE("shader_create(%s) - Failed to create lit mesh shader.", render_resource_rslt_to_str(ret));
        goto cleanup;
    }

    *out_lit_mesh_shader_ = tmp_lit_mesh_shader;
    tmp_lit_mesh_shader = NULL;

    ret = RENDER_RESOURCE_SUCCESS;

cleanup:
    if(RENDER_RESOURCE_DATA_CORRUPTED != ret) {
        fs_path_destroy(&vertex_shader_path);
        fs_path_destroy(&fragment_shader_path);
        if(NULL != tmp_lit_mesh_shader) {
            lit_mesh_shader_destroy(&tmp_lit_mesh_shader);
        }
    }

    return ret;
}

static bool is_valid_shallow(const lit_mesh_render_resource_t* render_resource_) {
    if(NULL == render_resource_) {
        return false;
    }
    if(NULL == render_resource_->geometry_registry) {
        return false;
    }
    if(NULL == render_resource_->shader) {
        return false;
    }
    return true;
}
