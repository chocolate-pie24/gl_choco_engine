#include "engine/systems/renderer/render_packet/render_packet.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/frame_allocator/frame_allocator.h"
#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/render_packet/core/render_packet_types.h"
#include "engine/systems/renderer/render_packet/core/render_packet_err_utils.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/point_mesh.h"
#include "engine/systems/renderer/resources/meshes/ui_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"

typedef struct {
    size_t max_mesh_count;
    size_t mesh_count;
    size_t read_index;
    line_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} line_mesh_render_packet_t;

typedef struct {
    size_t max_mesh_count;
    size_t mesh_count;
    size_t read_index;
    point_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} point_mesh_render_packet_t;

typedef struct {
    size_t max_mesh_count;
    size_t mesh_count;
    size_t read_index;
    ui_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} ui_mesh_render_packet_t;

typedef struct {
    size_t max_mesh_count;
    size_t mesh_count;
    size_t read_index;
    untextured_mesh_t* meshes;
    mat4x4f_t* model_matrices;
} untextured_mesh_render_packet_t;

struct render_packet {
    frame_allocator_t* allocator;

    line_mesh_render_packet_t line_mesh_packet;
    point_mesh_render_packet_t point_mesh_packet;
    ui_mesh_render_packet_t ui_mesh_packet;
    untextured_mesh_render_packet_t untextured_mesh_packet;
};

static bool render_packet_is_empty(const render_packet_t* render_packet_);
static bool line_mesh_packet_is_empty(const line_mesh_render_packet_t* render_packet_);
static bool point_mesh_packet_is_empty(const point_mesh_render_packet_t* render_packet_);
static bool ui_mesh_packet_is_empty(const ui_mesh_render_packet_t* render_packet_);
static bool untextured_mesh_packet_is_empty(const untextured_mesh_render_packet_t* render_packet_);

static bool mesh_packet_storage_is_valid(size_t max_mesh_count_, size_t mesh_count_, size_t read_index_, const void* meshes_, const mat4x4f_t* model_matrices_, size_t mesh_size_);

render_packet_result_t render_packet_create(size_t max_memory_pool_size_, render_packet_t** out_render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    frame_allocator_result_t ret_frame_allocator = FRAME_ALLOCATOR_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    render_packet_t* tmp_render_packet = NULL;
    frame_allocator_t* tmp_frame_allocator = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(out_render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_create", "out_render_packet_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_render_packet_, ret, RENDER_PACKET_BAD_OPERATION, render_packet_result_to_str(RENDER_PACKET_BAD_OPERATION), "render_packet_create", "*out_render_packet_")
    if(0 == max_memory_pool_size_) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_create(%s) - Provided max_memory_pool_size_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(render_packet_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_render_packet);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_packet_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = RENDER_PACKET_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_create(%s) - general_allocator_allocate failed.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    ret_frame_allocator = frame_allocator_create(max_memory_pool_size_, &tmp_frame_allocator);
    if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
        ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
        if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
            ret = RENDER_PACKET_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("render_packet_create(%s) - frame_allocator_create failed.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    tmp_render_packet->allocator = tmp_frame_allocator;
    render_packet_deinitialize(tmp_render_packet);

    // Result validation
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid((tmp_render_packet))) {
        ret = RENDER_PACKET_DATA_CORRUPTED;
        ERROR_MESSAGE("render_packet_create(%s) - Result validation failed for '*out_render_packet_'.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_render_packet_ = tmp_render_packet;
    tmp_render_packet = NULL;
    tmp_frame_allocator = NULL;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    if(RENDER_PACKET_DATA_CORRUPTED == ret || RENDER_PACKET_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_frame_allocator) {
        frame_allocator_destroy(&tmp_frame_allocator);
    }
    if(NULL != tmp_render_packet) {
        general_allocator_free((void**)&tmp_render_packet, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

void render_packet_destroy(render_packet_t** render_packet_) {
    if(NULL == render_packet_ || NULL == *render_packet_) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(*render_packet_)) {
        ERROR_MESSAGE("render_packet_destroy(%s) - Precondition validation failed for '*render_packet_'.", render_packet_result_to_str(RENDER_PACKET_DATA_CORRUPTED));
        return;
    }
#endif

    frame_allocator_destroy(&(*render_packet_)->allocator);
    general_allocator_free((void**)render_packet_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

// 失敗したら全部破棄する前提なので当面はrollbackは行わない
// 失敗時にrender_packet_は不変ではない場合がある
render_packet_result_t render_packet_initialize(size_t max_line_mesh_count_, size_t max_point_mesh_count_, size_t max_ui_mesh_count_, size_t max_untextured_mesh_count_, render_packet_t* render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    frame_allocator_result_t ret_frame_allocator = FRAME_ALLOCATOR_INVALID_ARGUMENT;

    line_mesh_t* tmp_line_meshes = NULL;
    point_mesh_t* tmp_point_meshes = NULL;
    ui_mesh_t* tmp_ui_meshes = NULL;
    untextured_mesh_t* tmp_untextured_meshes = NULL;

    mat4x4f_t* tmp_line_mesh_model_matrices = NULL;
    mat4x4f_t* tmp_point_mesh_model_matrices = NULL;
    mat4x4f_t* tmp_ui_mesh_model_matrices = NULL;
    mat4x4f_t* tmp_untextured_mesh_model_matrices = NULL;

    size_t allocation_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_initialize", "render_packet_")
    if(!render_packet_is_empty(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_initialize(%s) - Provided render_packet_ is not empty.", render_packet_result_to_str(ret));
        goto cleanup;
    }
    if(NULL == render_packet_->allocator) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_initialize(%s) - render_packet_->allocator is NULL.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_initialize(%s) - Precondition validation failed for 'render_packet_'.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Line mesh.
    if(0 != max_line_mesh_count_) {
        if(!size_mul_checked(sizeof(line_mesh_t), max_line_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_line_meshes);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(mat4x4f_t), max_line_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_line_mesh_model_matrices);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }
    }

    // Point mesh.
    if(0 != max_point_mesh_count_) {
        if(!size_mul_checked(sizeof(point_mesh_t), max_point_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_point_meshes);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(mat4x4f_t), max_point_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_point_mesh_model_matrices);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }
    }

    // UI mesh.
    if(0 != max_ui_mesh_count_) {
        if(!size_mul_checked(sizeof(ui_mesh_t), max_ui_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_ui_meshes);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(mat4x4f_t), max_ui_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_ui_mesh_model_matrices);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }
    }

    // Untextured mesh.
    if(0 != max_untextured_mesh_count_) {
        if(!size_mul_checked(sizeof(untextured_mesh_t), max_untextured_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_untextured_meshes);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }

        if(!size_mul_checked(sizeof(mat4x4f_t), max_untextured_mesh_count_, &allocation_size)) {
            ret = RENDER_PACKET_OVERFLOW;
            ERROR_MESSAGE("render_packet_initialize(%s) - render packet allocation overflow.", render_packet_result_to_str(ret));
            goto cleanup;
        }
        ret_frame_allocator = frame_allocator_allocate(render_packet_->allocator, allocation_size, FRAME_ALLOCATOR_MEMORY_TAG_RENDER_PACKET, (void**)&tmp_untextured_mesh_model_matrices);
        if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
            ret = render_packet_result_convert_frame_allocator(ret_frame_allocator);
            if(FRAME_ALLOCATOR_INVALID_ARGUMENT == ret_frame_allocator) {
                ret = RENDER_PACKET_DATA_CORRUPTED;
            }
            ERROR_MESSAGE("render_packet_initialize(%s) - frame_allocator_allocate failed.", render_packet_result_to_str(ret));
            goto cleanup;
        }
    }

    // Output.
    render_packet_->line_mesh_packet.max_mesh_count = max_line_mesh_count_;
    render_packet_->line_mesh_packet.mesh_count = 0;
    render_packet_->line_mesh_packet.read_index = 0;
    render_packet_->line_mesh_packet.meshes = tmp_line_meshes;
    render_packet_->line_mesh_packet.model_matrices = tmp_line_mesh_model_matrices;

    render_packet_->point_mesh_packet.max_mesh_count = max_point_mesh_count_;
    render_packet_->point_mesh_packet.mesh_count = 0;
    render_packet_->point_mesh_packet.read_index = 0;
    render_packet_->point_mesh_packet.meshes = tmp_point_meshes;
    render_packet_->point_mesh_packet.model_matrices = tmp_point_mesh_model_matrices;

    render_packet_->ui_mesh_packet.max_mesh_count = max_ui_mesh_count_;
    render_packet_->ui_mesh_packet.mesh_count = 0;
    render_packet_->ui_mesh_packet.read_index = 0;
    render_packet_->ui_mesh_packet.meshes = tmp_ui_meshes;
    render_packet_->ui_mesh_packet.model_matrices = tmp_ui_mesh_model_matrices;

    render_packet_->untextured_mesh_packet.max_mesh_count = max_untextured_mesh_count_;
    render_packet_->untextured_mesh_packet.mesh_count = 0;
    render_packet_->untextured_mesh_packet.read_index = 0;
    render_packet_->untextured_mesh_packet.meshes = tmp_untextured_meshes;
    render_packet_->untextured_mesh_packet.model_matrices = tmp_untextured_mesh_model_matrices;

    // Postcondition
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_DATA_CORRUPTED;
        ERROR_MESSAGE("render_packet_initialize(%s) - Postcondition validation failed for 'render_packet_'.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

void render_packet_deinitialize(render_packet_t* render_packet_) {
    frame_allocator_result_t ret_frame_allocator = FRAME_ALLOCATOR_INVALID_ARGUMENT;

    if(NULL == render_packet_ || NULL == render_packet_->allocator) {
        return;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ERROR_MESSAGE("render_packet_deinitialize(%s) - Precondition validation failed for 'render_packet_'.", render_packet_result_to_str(RENDER_PACKET_DATA_CORRUPTED));
        return;
    }
#endif
    ret_frame_allocator = frame_allocator_reset(render_packet_->allocator);
    if(FRAME_ALLOCATOR_SUCCESS != ret_frame_allocator) {
        ERROR_MESSAGE("render_packet_deinitialize(%s) - frame_allocator_reset failed.", render_packet_result_to_str(RENDER_PACKET_DATA_CORRUPTED));
        return;
    }

    render_packet_->line_mesh_packet.max_mesh_count = 0;
    render_packet_->line_mesh_packet.mesh_count = 0;
    render_packet_->line_mesh_packet.read_index = 0;
    render_packet_->line_mesh_packet.meshes = NULL;
    render_packet_->line_mesh_packet.model_matrices = NULL;

    render_packet_->point_mesh_packet.max_mesh_count = 0;
    render_packet_->point_mesh_packet.mesh_count = 0;
    render_packet_->point_mesh_packet.read_index = 0;
    render_packet_->point_mesh_packet.meshes = NULL;
    render_packet_->point_mesh_packet.model_matrices = NULL;

    render_packet_->ui_mesh_packet.max_mesh_count = 0;
    render_packet_->ui_mesh_packet.mesh_count = 0;
    render_packet_->ui_mesh_packet.read_index = 0;
    render_packet_->ui_mesh_packet.meshes = NULL;
    render_packet_->ui_mesh_packet.model_matrices = NULL;

    render_packet_->untextured_mesh_packet.max_mesh_count = 0;
    render_packet_->untextured_mesh_packet.mesh_count = 0;
    render_packet_->untextured_mesh_packet.read_index = 0;
    render_packet_->untextured_mesh_packet.meshes = NULL;
    render_packet_->untextured_mesh_packet.model_matrices = NULL;
}

void render_packet_read_state_reset(render_packet_t* render_packet_) {
    if(NULL == render_packet_) {
        return;
    }
    render_packet_->line_mesh_packet.read_index = 0;
    render_packet_->point_mesh_packet.read_index = 0;
    render_packet_->ui_mesh_packet.read_index = 0;
    render_packet_->untextured_mesh_packet.read_index = 0;
}

render_packet_result_t render_packet_line_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    bool tmp_completed = false;

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_line_mesh_read_state_completed", "render_packet_")
    IF_ARG_NULL_GOTO_CLEANUP(out_completed_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_line_mesh_read_state_completed", "out_completed_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_line_mesh_read_state_completed(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->line_mesh_packet.mesh_count == render_packet_->line_mesh_packet.read_index) {
        tmp_completed = true;
    }
    *out_completed_ = tmp_completed;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_point_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    bool tmp_completed = false;

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_point_mesh_read_state_completed", "render_packet_")
    IF_ARG_NULL_GOTO_CLEANUP(out_completed_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_point_mesh_read_state_completed", "out_completed_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_point_mesh_read_state_completed(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->point_mesh_packet.mesh_count == render_packet_->point_mesh_packet.read_index) {
        tmp_completed = true;
    }
    *out_completed_ = tmp_completed;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_ui_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    bool tmp_completed = false;

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_ui_mesh_read_state_completed", "render_packet_")
    IF_ARG_NULL_GOTO_CLEANUP(out_completed_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_ui_mesh_read_state_completed", "out_completed_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_ui_mesh_read_state_completed(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->ui_mesh_packet.mesh_count == render_packet_->ui_mesh_packet.read_index) {
        tmp_completed = true;
    }
    *out_completed_ = tmp_completed;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_untextured_mesh_read_state_completed(const render_packet_t* render_packet_, bool* out_completed_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    bool tmp_completed = false;

    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_untextured_mesh_read_state_completed", "render_packet_")
    IF_ARG_NULL_GOTO_CLEANUP(out_completed_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_untextured_mesh_read_state_completed", "out_completed_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_untextured_mesh_read_state_completed(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->untextured_mesh_packet.mesh_count == render_packet_->untextured_mesh_packet.read_index) {
        tmp_completed = true;
    }
    *out_completed_ = tmp_completed;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_line_mesh_push(const line_mesh_t* mesh_, const mat4x4f_t* model_matrix_, render_packet_t* render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_line_mesh_push", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_line_mesh_push", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_line_mesh_push", "render_packet_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_line_mesh_push(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->line_mesh_packet.max_mesh_count == render_packet_->line_mesh_packet.mesh_count) {
        ret = RENDER_PACKET_LIMIT_EXCEEDED;
        ERROR_MESSAGE("render_packet_line_mesh_push(%s) - Line mesh storage is full.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    render_packet_->line_mesh_packet.meshes[render_packet_->line_mesh_packet.mesh_count] = *mesh_;
    render_packet_->line_mesh_packet.model_matrices[render_packet_->line_mesh_packet.mesh_count] = *model_matrix_;
    render_packet_->line_mesh_packet.mesh_count++;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_point_mesh_push(const point_mesh_t* mesh_, const mat4x4f_t* model_matrix_, render_packet_t* render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_point_mesh_push", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_point_mesh_push", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_point_mesh_push", "render_packet_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_point_mesh_push(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->point_mesh_packet.max_mesh_count == render_packet_->point_mesh_packet.mesh_count) {
        ret = RENDER_PACKET_LIMIT_EXCEEDED;
        ERROR_MESSAGE("render_packet_point_mesh_push(%s) - Point mesh storage is full.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    render_packet_->point_mesh_packet.meshes[render_packet_->point_mesh_packet.mesh_count] = *mesh_;
    render_packet_->point_mesh_packet.model_matrices[render_packet_->point_mesh_packet.mesh_count] = *model_matrix_;
    render_packet_->point_mesh_packet.mesh_count++;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_ui_mesh_push(const ui_mesh_t* mesh_, const mat4x4f_t* model_matrix_, render_packet_t* render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_ui_mesh_push", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_ui_mesh_push", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_ui_mesh_push", "render_packet_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_ui_mesh_push(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->ui_mesh_packet.max_mesh_count == render_packet_->ui_mesh_packet.mesh_count) {
        ret = RENDER_PACKET_LIMIT_EXCEEDED;
        ERROR_MESSAGE("render_packet_ui_mesh_push(%s) - UI mesh storage is full.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    render_packet_->ui_mesh_packet.meshes[render_packet_->ui_mesh_packet.mesh_count] = *mesh_;
    render_packet_->ui_mesh_packet.model_matrices[render_packet_->ui_mesh_packet.mesh_count] = *model_matrix_;
    render_packet_->ui_mesh_packet.mesh_count++;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

render_packet_result_t render_packet_untextured_mesh_push(const untextured_mesh_t* mesh_, const mat4x4f_t* model_matrix_, render_packet_t* render_packet_) {
    render_packet_result_t ret = RENDER_PACKET_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(mesh_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_untextured_mesh_push", "mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(model_matrix_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_untextured_mesh_push", "model_matrix_")
    IF_ARG_NULL_GOTO_CLEANUP(render_packet_, ret, RENDER_PACKET_INVALID_ARGUMENT, render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT), "render_packet_untextured_mesh_push", "render_packet_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ret = RENDER_PACKET_INVALID_ARGUMENT;
        ERROR_MESSAGE("render_packet_untextured_mesh_push(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(ret));
        goto cleanup;
    }
#endif

    if(render_packet_->untextured_mesh_packet.max_mesh_count == render_packet_->untextured_mesh_packet.mesh_count) {
        ret = RENDER_PACKET_LIMIT_EXCEEDED;
        ERROR_MESSAGE("render_packet_untextured_mesh_push(%s) - Untextured mesh storage is full.", render_packet_result_to_str(ret));
        goto cleanup;
    }

    render_packet_->untextured_mesh_packet.meshes[render_packet_->untextured_mesh_packet.mesh_count] = *mesh_;
    render_packet_->untextured_mesh_packet.model_matrices[render_packet_->untextured_mesh_packet.mesh_count] = *model_matrix_;
    render_packet_->untextured_mesh_packet.mesh_count++;

    ret = RENDER_PACKET_SUCCESS;

cleanup:
    return ret;
}

bool render_packet_line_mesh_read_next(render_packet_t* render_packet_, line_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_) {
    if(NULL == render_packet_ || NULL == out_mesh_ || NULL == out_model_matrix_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ERROR_MESSAGE("render_packet_line_mesh_read_next(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT));
        return false;
    }
#endif
    if(render_packet_->line_mesh_packet.mesh_count == render_packet_->line_mesh_packet.read_index) {
        return false;
    }
    *out_mesh_ = render_packet_->line_mesh_packet.meshes[render_packet_->line_mesh_packet.read_index];
    *out_model_matrix_ = render_packet_->line_mesh_packet.model_matrices[render_packet_->line_mesh_packet.read_index];
    render_packet_->line_mesh_packet.read_index++;
    return true;
}

bool render_packet_point_mesh_read_next(render_packet_t* render_packet_, point_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_) {
    if(NULL == render_packet_ || NULL == out_mesh_ || NULL == out_model_matrix_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ERROR_MESSAGE("render_packet_point_mesh_read_next(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT));
        return false;
    }
#endif
    if(render_packet_->point_mesh_packet.mesh_count == render_packet_->point_mesh_packet.read_index) {
        return false;
    }
    *out_mesh_ = render_packet_->point_mesh_packet.meshes[render_packet_->point_mesh_packet.read_index];
    *out_model_matrix_ = render_packet_->point_mesh_packet.model_matrices[render_packet_->point_mesh_packet.read_index];
    render_packet_->point_mesh_packet.read_index++;
    return true;
}

bool render_packet_ui_mesh_read_next(render_packet_t* render_packet_, ui_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_) {
    if(NULL == render_packet_ || NULL == out_mesh_ || NULL == out_model_matrix_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ERROR_MESSAGE("render_packet_ui_mesh_read_next(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT));
        return false;
    }
#endif
    if(render_packet_->ui_mesh_packet.mesh_count == render_packet_->ui_mesh_packet.read_index) {
        return false;
    }
    *out_mesh_ = render_packet_->ui_mesh_packet.meshes[render_packet_->ui_mesh_packet.read_index];
    *out_model_matrix_ = render_packet_->ui_mesh_packet.model_matrices[render_packet_->ui_mesh_packet.read_index];
    render_packet_->ui_mesh_packet.read_index++;
    return true;
}

bool render_packet_untextured_mesh_read_next(render_packet_t* render_packet_, untextured_mesh_t* out_mesh_, mat4x4f_t* out_model_matrix_) {
    if(NULL == render_packet_ || NULL == out_mesh_ || NULL == out_model_matrix_) {
        return false;
    }
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!render_packet_is_valid(render_packet_)) {
        ERROR_MESSAGE("render_packet_untextured_mesh_read_next(%s) - Provided render_packet_ is not valid.", render_packet_result_to_str(RENDER_PACKET_INVALID_ARGUMENT));
        return false;
    }
#endif
    if(render_packet_->untextured_mesh_packet.mesh_count == render_packet_->untextured_mesh_packet.read_index) {
        return false;
    }
    *out_mesh_ = render_packet_->untextured_mesh_packet.meshes[render_packet_->untextured_mesh_packet.read_index];
    *out_model_matrix_ = render_packet_->untextured_mesh_packet.model_matrices[render_packet_->untextured_mesh_packet.read_index];
    render_packet_->untextured_mesh_packet.read_index++;
    return true;
}

bool render_packet_is_valid(const render_packet_t* render_packet_) {
    if(NULL == render_packet_) {
        return false;
    }

    // Frame Allocator ownership.
    if(NULL == render_packet_->allocator) {
        return false;
    }

    if(!general_allocator_ptr_is_allocated(render_packet_->allocator)) {
        return false;
    }

    if(!frame_allocator_is_valid(render_packet_->allocator)) {
        return false;
    }

    // Line Mesh Packet.
    if(!mesh_packet_storage_is_valid(
        render_packet_->line_mesh_packet.max_mesh_count,
        render_packet_->line_mesh_packet.mesh_count,
        render_packet_->line_mesh_packet.read_index,
        render_packet_->line_mesh_packet.meshes,
        render_packet_->line_mesh_packet.model_matrices,
        sizeof(line_mesh_t))) {
        return false;
    }

    // Point Mesh Packet.
    if(!mesh_packet_storage_is_valid(
        render_packet_->point_mesh_packet.max_mesh_count,
        render_packet_->point_mesh_packet.mesh_count,
        render_packet_->point_mesh_packet.read_index,
        render_packet_->point_mesh_packet.meshes,
        render_packet_->point_mesh_packet.model_matrices,
        sizeof(point_mesh_t))) {
        return false;
    }

    // UI Mesh Packet.
    if(!mesh_packet_storage_is_valid(
        render_packet_->ui_mesh_packet.max_mesh_count,
        render_packet_->ui_mesh_packet.mesh_count,
        render_packet_->ui_mesh_packet.read_index,
        render_packet_->ui_mesh_packet.meshes,
        render_packet_->ui_mesh_packet.model_matrices,
        sizeof(ui_mesh_t))) {
        return false;
    }

    // Untextured Mesh Packet.
    if(!mesh_packet_storage_is_valid(
        render_packet_->untextured_mesh_packet.max_mesh_count,
        render_packet_->untextured_mesh_packet.mesh_count,
        render_packet_->untextured_mesh_packet.read_index,
        render_packet_->untextured_mesh_packet.meshes,
        render_packet_->untextured_mesh_packet.model_matrices,
        sizeof(untextured_mesh_t))) {
        return false;
    }

    return true;
}

static bool render_packet_is_empty(const render_packet_t* render_packet_) {
    if(!line_mesh_packet_is_empty(&render_packet_->line_mesh_packet)) {
        return false;
    }
    if(!point_mesh_packet_is_empty(&render_packet_->point_mesh_packet)) {
        return false;
    }
    if(!ui_mesh_packet_is_empty(&render_packet_->ui_mesh_packet)) {
        return false;
    }
    if(!untextured_mesh_packet_is_empty(&render_packet_->untextured_mesh_packet)) {
        return false;
    }
    return true;
}

static bool line_mesh_packet_is_empty(const line_mesh_render_packet_t* render_packet_) {
    if(0 == render_packet_->max_mesh_count && 0 == render_packet_->mesh_count && NULL == render_packet_->meshes && NULL == render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool point_mesh_packet_is_empty(const point_mesh_render_packet_t* render_packet_) {
    if(0 == render_packet_->max_mesh_count && 0 == render_packet_->mesh_count && NULL == render_packet_->meshes && NULL == render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool ui_mesh_packet_is_empty(const ui_mesh_render_packet_t* render_packet_) {
    if(0 == render_packet_->max_mesh_count && 0 == render_packet_->mesh_count && NULL == render_packet_->meshes && NULL == render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool untextured_mesh_packet_is_empty(const untextured_mesh_render_packet_t* render_packet_) {
    if(0 == render_packet_->max_mesh_count && 0 == render_packet_->mesh_count && NULL == render_packet_->meshes && NULL == render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool mesh_packet_storage_is_valid(size_t max_mesh_count_, size_t mesh_count_, size_t read_index_, const void* meshes_, const mat4x4f_t* model_matrices_, size_t mesh_size_) {
    if(mesh_count_ > max_mesh_count_) {
        return false;
    }
    if(0 == max_mesh_count_) {
        if(0 != mesh_count_ || 0 != read_index_ || NULL != meshes_ || NULL != model_matrices_) {
            return false;
        }
        return true;
    }
    if(NULL == meshes_ || NULL == model_matrices_) {
        return false;
    }
    if(max_mesh_count_ > SIZE_MAX / mesh_size_) {
        return false;
    }
    if(max_mesh_count_ > SIZE_MAX / sizeof(mat4x4f_t)) {
        return false;
    }
    if(read_index_ > mesh_count_) {
        return false;
    }

    return true;
}
