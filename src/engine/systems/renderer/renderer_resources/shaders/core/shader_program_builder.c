#include "engine/systems/renderer/renderer_resources/shaders/core/shader_program_builder.h"

#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils.h"

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/renderer_backend/core/renderer_backend_types.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context/context_shader.h"

#include "engine/systems/renderer/renderer_resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/renderer_resources/shaders/core/shader_err_utils.h"

static shader_result_t shader_source_load(const char* file_path_, const char* name_, const char* extension_, choco_string_t** out_shader_source_);
static shader_result_t shader_program_build(renderer_backend_shader_t* shader_, renderer_backend_context_t* backend_context_, const choco_string_t* vertex_shader_source_, const choco_string_t* fragment_shader_source_);

shader_result_t shader_program_builder_create_from_files(renderer_backend_context_t* backend_context_, const char* file_path_, const char* name_, renderer_backend_shader_t** out_shader_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    choco_string_t* vert_shader_source = NULL;
    choco_string_t* frag_shader_source = NULL;

    renderer_backend_shader_t* tmp_shader = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_builder_create_from_files", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_builder_create_from_files", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_builder_create_from_files", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_builder_create_from_files", "out_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_builder_create_from_files", "*out_shader_")

    // シェーダーソースロード
    ret = shader_source_load(file_path_, name_, ".frag", &frag_shader_source);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("shader_program_builder_create_from_files(%s) - Failed to load fragment shader source.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = shader_source_load(file_path_, name_, ".vert", &vert_shader_source);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("shader_program_builder_create_from_files(%s) - Failed to load vertex shader source.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーハンドル生成
    ret_renderer_backend = renderer_backend_shader_create(backend_context_, &tmp_shader);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("shader_program_builder_create_from_files(%s) - Failed to create shader.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムビルド
    ret = shader_program_build(tmp_shader, backend_context_, vert_shader_source, frag_shader_source);
    if(SHADER_SUCCESS != ret) {
        ERROR_MESSAGE("shader_program_builder_create_from_files(%s) - Failed to build shader program.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    *out_shader_ = tmp_shader;

    ret = SHADER_SUCCESS;

cleanup:
    choco_string_destroy(&vert_shader_source);
    choco_string_destroy(&frag_shader_source);
    if(SHADER_SUCCESS != ret && NULL != tmp_shader) {
        renderer_backend_shader_destroy(backend_context_, &tmp_shader);
    }
    return ret;
}

static shader_result_t shader_source_load(const char* file_path_, const char* name_, const char* extension_, choco_string_t** out_shader_source_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;

    fs_utils_t* fs_utils = NULL;
    choco_string_t* shader_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_source_load", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_source_load", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_source_load", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(out_shader_source_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_source_load", "out_shader_source_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_shader_source_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_source_load", "*out_shader_source_")

    // シェーダーソース格納用choco_string生成
    ret_string = choco_string_default_create(&shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = shader_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("shader_source_load(%s) - Failed to create string for shader_source.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソース読み込み用fs_utils生成
    ret_fs_utils = fs_utils_create(file_path_, name_, extension_, FILESYSTEM_MODE_READ, &fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = shader_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("shader_source_load(%s) - Failed to create fs_utils for shader_source.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムロード
    ret_fs_utils = fs_utils_text_file_read(fs_utils, shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = shader_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("shader_source_load(%s) - Failed to read shader source.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    fs_utils_destroy(&fs_utils);

    *out_shader_source_ = shader_source;

    ret = SHADER_SUCCESS;

cleanup:
    if(SHADER_SUCCESS != ret) {
        if(NULL != fs_utils) {
            fs_utils_destroy(&fs_utils);
        }
        if(NULL != shader_source) {
            choco_string_destroy(&shader_source);
        }
    }
    return ret;
}

static shader_result_t shader_program_build(renderer_backend_shader_t* shader_, renderer_backend_context_t* backend_context_, const choco_string_t* vertex_shader_source_, const choco_string_t* fragment_shader_source_) {
    shader_result_t ret = SHADER_INVALID_ARGUMENT;

    renderer_backend_result_t ret_renderer_backend = RENDERER_BACKEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_build", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(shader_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_build", "shader_")
    IF_ARG_NULL_GOTO_CLEANUP(vertex_shader_source_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_build", "vertex_shader_source_")
    IF_ARG_NULL_GOTO_CLEANUP(fragment_shader_source_, ret, SHADER_INVALID_ARGUMENT, shader_rslt_to_str(SHADER_INVALID_ARGUMENT), "shader_program_build", "fragment_shader_source_")

    ret_renderer_backend = renderer_backend_shader_compile(SHADER_TYPE_VERTEX, choco_string_c_str(vertex_shader_source_), backend_context_, shader_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("shader_program_build(%s) - Failed to compile shader object(vertex_shader).", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_compile(SHADER_TYPE_FRAGMENT, choco_string_c_str(fragment_shader_source_), backend_context_, shader_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("shader_program_build(%s) - Failed to compile shader object(fragment_shader).", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret_renderer_backend = renderer_backend_shader_link(backend_context_, shader_);
    if(RENDERER_BACKEND_SUCCESS != ret_renderer_backend) {
        ret = shader_rslt_convert_renderer_backend(ret_renderer_backend);
        ERROR_MESSAGE("shader_program_build(%s) - Failed to link shader program.", shader_rslt_to_str(ret));
        goto cleanup;
    }

    ret = SHADER_SUCCESS;

cleanup:
    return ret;
}
