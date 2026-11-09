#include "engine/systems/renderer/render_packet/render_packet.h"

#include <stddef.h>
#include <stdbool.h>

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

static bool render_packet_is_empty(const render_packet_t* render_packet_);
static bool line_mesh_packet_is_empty(const line_mesh_render_packet_t* render_packet_);
static bool point_mesh_packet_is_empty(const point_mesh_render_packet_t* render_packet_);
static bool ui_mesh_packet_is_empty(const ui_mesh_render_packet_t* render_packet_);
static bool untextured_mesh_packet_is_empty(const untextured_mesh_render_packet_t* render_packet_);

static bool render_packet_is_not_empty(const render_packet_t* render_packet_);
static bool line_mesh_packet_is_not_empty(const line_mesh_render_packet_t* render_packet_);
static bool point_mesh_packet_is_not_empty(const point_mesh_render_packet_t* render_packet_);
static bool ui_mesh_packet_is_not_empty(const ui_mesh_render_packet_t* render_packet_);
static bool untextured_mesh_packet_is_not_empty(const untextured_mesh_render_packet_t* render_packet_);

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

    tmp_render_packet->line_mesh_packet.max_mesh_count = 0;
    tmp_render_packet->line_mesh_packet.mesh_count = 0;
    tmp_render_packet->line_mesh_packet.meshes = NULL;
    tmp_render_packet->line_mesh_packet.model_matrices = NULL;

    tmp_render_packet->point_mesh_packet.max_mesh_count = 0;
    tmp_render_packet->point_mesh_packet.mesh_count = 0;
    tmp_render_packet->point_mesh_packet.meshes = NULL;
    tmp_render_packet->point_mesh_packet.model_matrices = NULL;

    tmp_render_packet->ui_mesh_packet.max_mesh_count = 0;
    tmp_render_packet->ui_mesh_packet.mesh_count = 0;
    tmp_render_packet->ui_mesh_packet.meshes = NULL;
    tmp_render_packet->ui_mesh_packet.model_matrices = NULL;

    tmp_render_packet->untextured_mesh_packet.max_mesh_count = 0;
    tmp_render_packet->untextured_mesh_packet.mesh_count = 0;
    tmp_render_packet->untextured_mesh_packet.meshes = NULL;
    tmp_render_packet->untextured_mesh_packet.model_matrices = NULL;

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
    render_packet_->line_mesh_packet.meshes = tmp_line_meshes;
    render_packet_->line_mesh_packet.model_matrices = tmp_line_mesh_model_matrices;

    render_packet_->point_mesh_packet.max_mesh_count = max_point_mesh_count_;
    render_packet_->point_mesh_packet.mesh_count = 0;
    render_packet_->point_mesh_packet.meshes = tmp_point_meshes;
    render_packet_->point_mesh_packet.model_matrices = tmp_point_mesh_model_matrices;

    render_packet_->ui_mesh_packet.max_mesh_count = max_ui_mesh_count_;
    render_packet_->ui_mesh_packet.mesh_count = 0;
    render_packet_->ui_mesh_packet.meshes = tmp_ui_meshes;
    render_packet_->ui_mesh_packet.model_matrices = tmp_ui_mesh_model_matrices;

    render_packet_->untextured_mesh_packet.max_mesh_count = max_untextured_mesh_count_;
    render_packet_->untextured_mesh_packet.mesh_count = 0;
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
    render_packet_->line_mesh_packet.meshes = NULL;
    render_packet_->line_mesh_packet.model_matrices = NULL;

    render_packet_->point_mesh_packet.max_mesh_count = 0;
    render_packet_->point_mesh_packet.mesh_count = 0;
    render_packet_->point_mesh_packet.meshes = NULL;
    render_packet_->point_mesh_packet.model_matrices = NULL;

    render_packet_->ui_mesh_packet.max_mesh_count = 0;
    render_packet_->ui_mesh_packet.mesh_count = 0;
    render_packet_->ui_mesh_packet.meshes = NULL;
    render_packet_->ui_mesh_packet.model_matrices = NULL;

    render_packet_->untextured_mesh_packet.max_mesh_count = 0;
    render_packet_->untextured_mesh_packet.mesh_count = 0;
    render_packet_->untextured_mesh_packet.meshes = NULL;
    render_packet_->untextured_mesh_packet.model_matrices = NULL;
}

bool render_packet_is_valid(const render_packet_t* render_packet_) {
    if(NULL == render_packet_) {
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

static bool render_packet_is_not_empty(const render_packet_t* render_packet_) {
    if(!line_mesh_packet_is_not_empty(&render_packet_->line_mesh_packet)) {
        return false;
    }
    if(!point_mesh_packet_is_not_empty(&render_packet_->point_mesh_packet)) {
        return false;
    }
    if(!ui_mesh_packet_is_not_empty(&render_packet_->ui_mesh_packet)) {
        return false;
    }
    if(!untextured_mesh_packet_is_not_empty(&render_packet_->untextured_mesh_packet)) {
        return false;
    }
    return true;
}

static bool line_mesh_packet_is_not_empty(const line_mesh_render_packet_t* render_packet_) {
    if(0 != render_packet_->max_mesh_count || 0 != render_packet_->mesh_count || NULL != render_packet_->meshes || NULL != render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool point_mesh_packet_is_not_empty(const point_mesh_render_packet_t* render_packet_) {
    if(0 != render_packet_->max_mesh_count || 0 != render_packet_->mesh_count || NULL != render_packet_->meshes || NULL != render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool ui_mesh_packet_is_not_empty(const ui_mesh_render_packet_t* render_packet_) {
    if(0 != render_packet_->max_mesh_count || 0 != render_packet_->mesh_count || NULL != render_packet_->meshes || NULL != render_packet_->model_matrices) {
        return true;
    }
    return false;
}

static bool untextured_mesh_packet_is_not_empty(const untextured_mesh_render_packet_t* render_packet_) {
    if(0 != render_packet_->max_mesh_count || 0 != render_packet_->mesh_count || NULL != render_packet_->meshes || NULL != render_packet_->model_matrices) {
        return true;
    }
    return false;
}
