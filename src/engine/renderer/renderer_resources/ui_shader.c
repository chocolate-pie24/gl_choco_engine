#include "engine/renderer/renderer_resources/ui_shader.h"

#include "engine/renderer/renderer_backend/renderer_backend_context/context.h"
#include "engine/renderer/renderer_backend/renderer_backend_context/context_shader.h"

#include "engine/renderer/renderer_core/renderer_err_utils.h"
#include "engine/renderer/renderer_core/renderer_memory.h"

#include "engine/renderer/renderer_backend/renderer_backend_types.h"

#include "engine/containers/choco_string.h"

#include "engine/io_utils/fs_utils/fs_utils.h"

#include "engine/core/memory/choco_memory.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

struct ui_shader {
    renderer_backend_shader_t* shader;
};

renderer_result_t ui_shader_create(const char* file_path_, const char* name_, const char* extension_, renderer_backend_context_t* backend_context_, ui_shader_t** out_ui_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;
    fs_utils_result_t ret_fs_utils = FS_UTILS_INVALID_ARGUMENT;
    memory_system_result_t ret_memory_system = MEMORY_SYSTEM_INVALID_ARGUMENT;

    ui_shader_t* tmp_ui_shader = NULL;

    fs_utils_t* frag_fs_utils = NULL;
    fs_utils_t* vert_fs_utils = NULL;
    choco_string_t* vert_shader_source = NULL;
    choco_string_t* frag_shader_source = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(file_path_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "file_path_")
    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(extension_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "extension_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(out_ui_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "out_ui_shader_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ui_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_create", "*out_ui_shader_")

    // ui shader構造体インスタンス生成
    ret = render_mem_allocate(sizeof(ui_shader_t), (void**)&tmp_ui_shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to allocate memory for tmp_ui_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーモジュール生成
    ret = renderer_backend_shader_create(backend_context_, &tmp_ui_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to create shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソース格納用choco_string生成
    ret_string = choco_string_default_create(&vert_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to create string for vert_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_string = choco_string_default_create(&frag_shader_source);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = renderer_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to create string for frag_shader_source.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーソース読み込み用fs_utils生成
    ret_fs_utils = fs_utils_create("assets/shaders/test_shader/", "fragment_shader", ".frag", FILESYSTEM_MODE_READ, &frag_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to create fs_utils for fragment_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret_fs_utils = fs_utils_create("assets/shaders/test_shader/", "vertex_shader", ".vert", FILESYSTEM_MODE_READ, &vert_fs_utils);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to create fs_utils for vertex_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダープログラムロード
    ret_fs_utils = fs_utils_text_file_read(frag_fs_utils, frag_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to read shader source(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }
    ret_fs_utils = fs_utils_text_file_read(vert_fs_utils, vert_shader_source);
    if(FS_UTILS_SUCCESS != ret_fs_utils) {
        ret = renderer_rslt_convert_fs_utils(ret_fs_utils);
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to read shader source(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    // シェーダーコンパイル / リンク
    ret = renderer_backend_shader_compile(SHADER_TYPE_VERTEX, choco_string_c_str(vert_shader_source), backend_context_, tmp_ui_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to compile shader object(vertex_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_compile(SHADER_TYPE_FRAGMENT, choco_string_c_str(frag_shader_source), backend_context_, tmp_ui_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to compile shader object(fragment_shader).", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    ret = renderer_backend_shader_link(backend_context_, tmp_ui_shader->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_create(%s) - Failed to link shader program.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

    choco_string_destroy(&vert_shader_source);
    choco_string_destroy(&frag_shader_source);
    fs_utils_destroy(&vert_fs_utils);
    fs_utils_destroy(&frag_fs_utils);

    *out_ui_shader_ = tmp_ui_shader;
    ret = RENDERER_SUCCESS;

cleanup:
    if(RENDERER_SUCCESS != ret) {
        if(NULL != vert_fs_utils) {
            fs_utils_destroy(&vert_fs_utils);
        }
        if(NULL != frag_fs_utils) {
            fs_utils_destroy(&frag_fs_utils);
        }
        if(NULL != frag_shader_source) {
            choco_string_destroy(&frag_shader_source);
        }
        if(NULL != vert_shader_source) {
            choco_string_destroy(&vert_shader_source);
        }
        if(NULL != tmp_ui_shader) {
            ui_shader_destroy(backend_context_, &tmp_ui_shader);
        }
    }
    return ret;
}

void ui_shader_destroy(renderer_backend_context_t* backend_context_, ui_shader_t** ui_shader_) {
    if(NULL == ui_shader_) {
        return;
    }
    if(NULL == *ui_shader_) {
        return;
    }
    if(NULL == backend_context_) {
        return;
    }
    if(NULL != (*ui_shader_)->shader) {
        renderer_backend_shader_destroy(backend_context_, &(*ui_shader_)->shader);
    }
    render_mem_free(*ui_shader_, sizeof(ui_shader_t));
    *ui_shader_ = NULL;
}

renderer_result_t ui_shader_use(renderer_backend_context_t* backend_context_, ui_shader_t* ui_shader_) {
    renderer_result_t ret = RENDERER_INVALID_ARGUMENT;
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_use", "backend_context_")
    IF_ARG_NULL_GOTO_CLEANUP(ui_shader_, ret, RENDERER_INVALID_ARGUMENT, renderer_rslt_to_str(RENDERER_INVALID_ARGUMENT), "ui_shader_use", "ui_shader_")

    ret = renderer_backend_shader_use(backend_context_, ui_shader_->shader);
    if(RENDERER_SUCCESS != ret) {
        ERROR_MESSAGE("ui_shader_use(%s) - Failed to switch program for ui_shader.", renderer_rslt_to_str(ret));
        goto cleanup;
    }

cleanup:
    return ret;
}
