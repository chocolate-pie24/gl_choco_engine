// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/render_model/model_transform_tree.h"

#include <stdint.h>
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/math_types.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/systems/renderer/resources/render_model/core/render_model_types.h"
#include "engine/systems/renderer/resources/render_model/core/render_model_err_utils.h"

/*
 * Module Internal Contract
 *
 * Tree Representation:
 * - Coordinateの階層構造はFirst-Child / Next-Sibling方式で表現する。
 * - 各Nodeのchildは最初のChildを、nextは同一Parentを持つ次のSiblingを指す。
 * - 各Nodeのparentは親Nodeへの非所有Referenceである。
 * - RootのparentとnextはNULLである。
 * - Childのparentは、そのChildを所有するParent Nodeと一致する。
 * - Sibling Chainに含まれる全Nodeのparentは一致する。
 * - 正常なTreeにはChild / Sibling参照によるCycleが存在しない。
 *
 * Internal State:
 * - Tree生成成功後、rootは常に存在する。
 * - 各Nodeは独立したAllocationとして保持され、Treeの破棄時に解放される。
 * - Treeの構造変更は本moduleのAPIだけが実行する。
 * - Coordinateの削除およびParentの変更は現在サポートしない。
 */

// ============================================================
// Private Type Definitions
// ============================================================
typedef struct model_transform_node {
    struct model_transform_node* parent;
    struct model_transform_node* child;
    struct model_transform_node* next;

    uint16_t coordinate_id; // Coordinate IDはModel内でのみ一意
    vec3f_t translate;
    vec3f_t rotation;
} model_transform_node_t;

struct model_transform_tree {
    model_transform_node_t* root;
};

// ============================================================
// Private Function Declarations
// ============================================================
static model_transform_node_t* node_find_by_id(model_transform_node_t* node_, uint16_t coordinate_id_);
static bool is_valid_shallow(const model_transform_tree_t* transform_tree_);

// ============================================================
// Public API
// ============================================================
/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - out_transform_tree_のNULLを全Buildで検査する。
 * - *out_transform_tree_ == NULLを確認し、既存ObjectへのPointerを
 *   上書きするAPI misuseを防ぐ。
 *
 * Result Validation:
 * - TreeとRoot Nodeの構築が完了した後、DEBUG / TEST BUILDで
 *   Canonical Validationを実行する。
 * - この検証は、構築したTreeをCallerへ公開する前に、
 *   Rootを含むEstablished Internal Stateの整合性を確認するために行う。
 * - RELEASE BUILDでは、構築処理とGeneral AllocatorのContractを信頼し、
 *   Canonical Result Validationを実行しない。
 *
 * Failure Handling:
 * - CallerへのOwnership Commit前に失敗した場合、
 *   DATA_CORRUPTED以外では確保済みStorageを解放する。
 * - DATA_CORRUPTEDの場合は、破損した可能性のあるStateを使用した
 *   Cleanupを行わない。
 */
render_model_result_t model_transform_tree_create(uint16_t coordinate_id_, model_transform_tree_t** out_transform_tree_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    model_transform_tree_t* tmp_tree = NULL;
    model_transform_node_t* tmp_root_node = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_create", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, RENDER_MODEL_BAD_OPERATION, render_model_result_to_str(RENDER_MODEL_BAD_OPERATION), "model_transform_tree_create", "*out_transform_tree_")

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(model_transform_tree_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_tree);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("model_transform_tree_create(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }

    ret_general_allocator = general_allocator_allocate(sizeof(model_transform_node_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_root_node);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = render_model_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("model_transform_tree_create(%s) - general_allocator_allocate failed.", render_model_result_to_str(ret));
        goto cleanup;
    }
    tmp_root_node->child = NULL;
    tmp_root_node->next = NULL;
    tmp_root_node->parent = NULL;

    tmp_root_node->coordinate_id = coordinate_id_;
    tmp_root_node->rotation = vec3f_initialize(0.0f, 0.0f, 0.0f);
    tmp_root_node->translate = vec3f_initialize(0.0f, 0.0f, 0.0f);

    tmp_tree->root = tmp_root_node;

    // Result validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(tmp_tree)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_create(%s) - Result validation failed for 'tmp_tree'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#endif

    *out_transform_tree_ = tmp_tree;
    tmp_root_node = NULL;
    tmp_tree = NULL;

    ret = RENDER_MODEL_SUCCESS;

cleanup:
    if(RENDER_MODEL_DATA_CORRUPTED != ret) {
        if(NULL != tmp_root_node) {
            general_allocator_free((void**)&tmp_root_node, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
        if(NULL != tmp_tree) {
            general_allocator_free((void**)&tmp_tree, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        }
    }
    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - transform_tree_および*transform_tree_のNULLは、
 *   破棄対象が存在しない状態として許容する。
 * - DEBUG / TEST BUILDでは、破棄前にCanonical Validationを実行する。
 *   これは、Parent / Child / Siblingの参照関係に依存した解放処理を
 *   正常なTree構造に対して実行するためである。
 * - RELEASE BUILDでは、Ownerから渡されたTreeのEstablished Internal Stateを信頼し、
 *   追加のStructural Validationを行わない。
 *
 * Postconditions:
 * - 破棄処理はTreeが所有するStorageの解放であり、
 *   解放後のObjectに対するValidationは行わない。
 * - General Allocatorによる有効なAllocationの解放成功は、
 *   下位Memory InfrastructureのContractとして信頼する。
 */
void model_transform_tree_destroy(model_transform_tree_t** transform_tree_) {
    model_transform_node_t* node = NULL;
    model_transform_node_t* parent = NULL;
    model_transform_node_t* next = NULL;

    if(NULL == transform_tree_) {
        return;
    }
    if(NULL == *transform_tree_) {
        return;
    }

#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(*transform_tree_)) {
        ERROR_MESSAGE("model_transform_tree_destroy(%s) - Preconditions validation failed for '*transform_tree_'.", render_model_result_to_str(RENDER_MODEL_DATA_CORRUPTED));
        return;
    }
#endif

    node = (*transform_tree_)->root;
    while(NULL != node) {
        if(NULL != node->child) {
            node = node->child;
            continue;
        }
        parent = node->parent;
        next = node->next;

        general_allocator_free((void**)&node, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        if(NULL == parent) {
            (*transform_tree_)->root = NULL;
            break;
        }
        parent->child = next;
        if(NULL != next) {
            node = next;
        } else {
            node = parent;
        }
    }
    general_allocator_free((void**)transform_tree_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - transform_tree_のNULLは全Buildで検査する。
 * - DEBUG / TEST BUILDではCanonical Validationを実行し、
 *   Node検索およびSibling Chainの操作に必要なTree構造の整合性を確認する。
 * - RELEASE BUILDではShallow Validationを実行し、
 *   Rootの存在を確認した上で内部構造を信頼する。
 * - Coordinate IDの重複は、新しいCoordinateのIdentityを確立するため
 *   全Buildで検査し、既存IDの再登録をBAD_OPERATIONとして扱う。
 * - Parent Coordinateの存在は、新規Nodeを接続する対象を確定するため
 *   全Buildで検査する。
 * - translate_ / rotation_の有限性は、信頼済み入力Contractとして扱い、
 *   個別のNaN / Infチェックは行わない。
 *
 * Postconditions:
 * - 新規NodeはTreeへの接続前に必要なFieldを初期化する。
 * - Commitでは既存Treeに対する限定的なLink更新のみを行い、
 *   既存NodeのParentやLocal Transformを変更しない。
 * - このMutationの性質と成立済みPreconditionを信頼し、
 *   Commit後のCanonical Validationは行わない。
 *
 * Failure Handling:
 * - Commit前の失敗では、DATA_CORRUPTEDを除き、
 *   新規確保したNodeを解放して既存Treeを維持する。
 * - DATA_CORRUPTEDの場合は、破損した可能性のあるStateに対する
 *   Cleanupを行わない。
 */
render_model_result_t model_transform_tree_add_child(model_transform_tree_t* transform_tree_, uint16_t parent_coordinate_id_, uint16_t coordinate_id_, vec3f_t translate_, vec3f_t rotation_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;

    model_transform_node_t* parent = NULL;
    model_transform_node_t* child = NULL;
    model_transform_node_t* next = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_add_child", "transform_tree_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#else
    if(!is_valid_shallow(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_add_child(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#endif
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

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - transform_tree_のNULLは全Buildで検査する。
 * - DEBUG / TEST BUILDではCanonical Validationを実行する。
 * - RELEASE BUILDではShallow Validationを実行し、
 *   Rootの存在を確認した上で内部構造を信頼する。
 * - 対象Coordinateの存在は、更新先を特定するため全Buildで確認する。
 * - rotation_の有限性は信頼済み入力Contractとして扱い、
 *   個別のNaN / Infチェックは行わない。
 *
 * Postconditions:
 * - Mutationは既存NodeのRotation Valueの代入に限定され、
 *   Treeの階層構造を変更しない。
 * - 成立済みPreconditionと単純代入の性質を信頼し、
 *   Postcondition Validationは行わない。
 */
render_model_result_t model_transform_tree_rotation_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t rotation_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    model_transform_node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_rotation_set", "transform_tree_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_rotation_set(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#else
    if(!is_valid_shallow(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_rotation_set(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#endif
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

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - transform_tree_のNULLは全Buildで検査する。
 * - DEBUG / TEST BUILDではCanonical Validationを実行する。
 * - RELEASE BUILDではShallow Validationを実行し、
 *   Rootの存在を確認した上で内部構造を信頼する。
 * - 対象Coordinateの存在は、更新先を特定するため全Buildで確認する。
 * - translate_の有限性は信頼済み入力Contractとして扱い、
 *   個別のNaN / Infチェックは行わない。
 *
 * Postconditions:
 * - Mutationは既存NodeのTranslation Valueの代入に限定され、
 *   Treeの階層構造を変更しない。
 * - 成立済みPreconditionと単純代入の性質を信頼し、
 *   Postcondition Validationは行わない。
 */
render_model_result_t model_transform_tree_translate_set(model_transform_tree_t* transform_tree_, uint16_t coordinate_id_, vec3f_t translate_) {
    render_model_result_t ret = RENDER_MODEL_INVALID_ARGUMENT;

    model_transform_node_t* node = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(transform_tree_, ret, RENDER_MODEL_INVALID_ARGUMENT, render_model_result_to_str(RENDER_MODEL_INVALID_ARGUMENT), "model_transform_tree_translate_set", "transform_tree_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_translate_set(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#else
    if(!is_valid_shallow(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_translate_set(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#endif
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

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - transform_tree_およびout_matrix_のNULLは全Buildで検査する。
 * - DEBUG / TEST BUILDではCanonical Validationを実行し、
 *   Coordinate検索とParent方向へのTraversalに必要な
 *   Tree構造の整合性を確認する。
 * - RELEASE BUILDではShallow Validationを実行し、
 *   Rootの存在を確認した上で内部構造を信頼する。
 * - 対象Coordinateの存在は、計算対象を特定するため全Buildで確認する。
 *
 * Result validation:
 * - Matrix計算には、成立済みのLocal Transform Contractと
 *   Choco MathのMatrix演算Contractを使用する。
 * - 新たなStructureやResourceを構築する処理ではないため、
 *   計算結果に対する追加のCanonical Validationは行わない。
 *
 * Postconditions:
 * - 本APIはTreeの内部Stateを変更しない。
 * - 計算結果は成功時にのみCallerのOutputへ格納するため、
 *   追加のPostcondition Validationは行わない。
 */
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
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!model_transform_tree_is_valid(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_world_matrix_get(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#else
    if(!is_valid_shallow(transform_tree_)) {
        ret = RENDER_MODEL_DATA_CORRUPTED;
        ERROR_MESSAGE("model_transform_tree_world_matrix_get(%s) - Preconditions validation failed for 'transform_tree_'.", render_model_result_to_str(ret));
        goto cleanup;
    }
#endif
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

/*
 * Canonical Validation Traversal Rationale
 *
 * - Treeの階層構造を、追加の動的Memory Allocationを行わずに検証する。
 * - 訪問済みNode集合やCoordinate IDのBitmapは保持せず、
 *   追加Memory使用量をO(1)に抑える。
 * - Coordinate IDがuint16_tであることから、有効なTreeが保持できる
 *   Node数の理論上限をUINT16_MAX + 1とする。
 * - Node訪問回数とParentへのBacktracking回数にそれぞれ上限を設け、
 *   Cycle等によってTraversalが無限に継続することを防ぐ。
 * - Nodeが指すStorageをDereferenceする前に、
 *   General Allocator上のAllocation生存状態を確認する。
 * - Allocation生存確認はPointerの参照先が正しいNode型やAllocation Sizeを
 *   持つことまで保証しないため、任意のMemory Corruptionに対する
 *   完全なMemory Safetyを保証するものではない。
 *
 * - 本Validatorは明示的に呼び出す場合、Buildによらず同じ検証を行う。
 *   DEBUG / TEST限定となるのは、他APIからのAutomatic Validationである。
 */
bool model_transform_tree_is_valid(const model_transform_tree_t* transform_tree_) {
    // Generated by ChatGPT
    const model_transform_node_t* root = NULL;
    const model_transform_node_t* node = NULL;
    const model_transform_node_t* next = NULL;

    const size_t max_nodes = (size_t)UINT16_MAX + 1U;

    size_t visited_count = 0;
    size_t backtrack_count = 0;

    if(NULL == transform_tree_) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(transform_tree_)) {
        return false;
    }
    if(!is_valid_shallow(transform_tree_)) {
        return false;
    }

    root = transform_tree_->root;
    if(!general_allocator_ptr_is_allocated(root)) {
        return false;
    }
    if(NULL != root->parent || NULL != root->next) {
        return false;
    }

    node = root;

    while(NULL != node) {
        if(++visited_count > max_nodes) {
            return false;
        }
        if(!general_allocator_ptr_is_allocated(node)) {
            return false;
        }
        if(!vec3f_is_finite(node->translate) || !vec3f_is_finite(node->rotation)) {
            return false;
        }

        if(NULL != node->child) {
            next = node->child;
            if(!general_allocator_ptr_is_allocated(next)) {
                return false;
            }
            if(next->parent != node) {
                return false;
            }
            node = next;
            continue;
        }

        while(node != root && NULL == node->next) {
            if(++backtrack_count > max_nodes) {
                return false;
            }
            next = node->parent;
            if(!general_allocator_ptr_is_allocated(next)) {
                return false;
            }
            node = next;
        }

        if(node == root) {
            return true;
        }

        next = node->next;
        if(!general_allocator_ptr_is_allocated(next)) {
            return false;
        }
        if(next->parent != node->parent) {
            return false;
        }
        node = next;
    }

    return false;
}

/*
 * node_find_by_id() Contract
 *
 * Preconditions:
 * - node_が非NULLの場合、有効なLifetime中のNodeを指す。
 * - node_以下のSubtreeは、Child / Sibling / Parentの参照関係が整合し、
 *   Cycleを含まない正常なTree構造である。
 * - 本Helperは上位APIで成立したInternal Contractを信頼し、
 *   Structural Validationを再実行しない。
 *
 * Responsibility:
 * - 指定されたCoordinate IDを持つNodeを、指定Node以下のSubtreeから検索する。
 * - 検索結果はTreeが所有するNodeへのBorrowed Pointerとして返す。
 * - 対象が存在しない場合はNULLを返す。
 *
 * Traversal Design:
 * - Child / Sibling / Parentを使用した非再帰Depth First Searchを行う。
 * - 開始NodeのSiblingには進まず、探索範囲をSubtree内に限定する。
 * - Tree Depthに依存するCall Stackの増加を避けるため、
 *   追加Memory使用量をO(1)とする。
 */
static model_transform_node_t* node_find_by_id(model_transform_node_t* node_, uint16_t coordinate_id_) {
    model_transform_node_t* root = NULL;
    if(NULL == node_) {
        return NULL;
    }

    root = node_;
    while(NULL != node_) {
        if(node_->coordinate_id == coordinate_id_) {
            return node_;
        }
        if(NULL != node_->child) {
            node_ = node_->child;
            continue;
        }
        while(node_ != root && NULL == node_->next) {
            node_ = node_->parent;
        }
        if(node_ == root) {
            break;
        }
        node_ = node_->next;
    }

    return NULL;
}

/*
 * is_valid_shallow() Validation
 *
 * Purpose:
 * - TreeのRootが存在することを確認し、
 *   後続処理がRootを起点としたOperationを開始できるようにする。
 *
 * Validation Scope:
 * - transform_tree_がNULLではないこと。
 * - transform_tree_->rootがNULLではないこと。
 * - Rootが指すStorageはDereferenceしない。
 * - Allocation生存状態やNode間のStructural Relationは検証しない。
 */
static bool is_valid_shallow(const model_transform_tree_t* transform_tree_) {
    if(NULL == transform_tree_) {
        return false;
    }
    if(NULL == transform_tree_->root) {
        return false;
    }
    return true;
}
