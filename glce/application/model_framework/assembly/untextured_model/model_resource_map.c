// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "application/model_framework/assembly/untextured_model/model_resource_map.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "config/build_config.h"

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/core/geometry_primitive/aabb_3d.h"
#include "engine/core/geometry_primitive/cone.h"
#include "engine/core/geometry_primitive/cylinder.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/containers/ring_queue.h"
#include "engine/containers/choco_string.h"

#include "engine/systems/renderer/resources/meshes/line_mesh.h"
#include "engine/systems/renderer/resources/meshes/untextured_mesh.h"
#include "engine/systems/renderer/resources/geometries/generators/untextured_cone_geometry.h"
#include "engine/systems/renderer/resources/geometries/generators/untextured_cylinder_geometry.h"
#include "engine/systems/renderer/resources/materials/material_types.h"

#include "engine/systems/renderer/renderer_frontend/core/renderer_frontend_types.h"
#include "engine/systems/renderer/renderer_frontend/renderer_frontend.h"

#include "application/model_framework/core/model_framework_types.h"
#include "application/model_framework/core/model_framework_err_utils.h"
#include "application/model_framework/assembly/untextured_model/model_assembly_config.h"
#include "application/model_framework/coordinates/model_coordinate_config.h"
#include "application/model_framework/coordinates/model_transform_tree.h"

/*
 * Module Internal Contract
 *
 * Canonical State:
 * - resource_map_nameはNULLではなく、General Allocator上のlive allocationである。
 * - resource_map_nameはcanonical validityを満たす空でないChoco Stringである。
 * - transform_treeはNULLではなく、General Allocator上のlive allocationである。
 * - transform_treeはModel Transform Treeとしてcanonical validityを満たす。
 *
 * Mesh Binding Representation:
 * - Mesh BindingはMesh配列とCoordinate ID配列の同一Indexによって表現する。
 * - 各Mesh群はMesh Count、Mesh配列、Coordinate ID配列を一組として保持する。
 * - Mesh配列とCoordinate ID配列は、それぞれ独立したAllocationである。
 * - 各配列は対応するMesh Count個の要素を保持できるStorageとして確保される。
 * - 各配列はNULLではなく、General Allocator上のlive allocationである。
 * - Mesh Countと各要素型のsizeofの積はsize_tで表現可能である。
 *
 * Mesh Count Invariants:
 * - untextured_mesh_countは0より大きい。
 * - debug_aabb_mesh_countはuntextured_mesh_countと等しい。
 * - axis_mesh_countは0より大きく、3の倍数である。
 *
 * Mesh Binding Construction:
 * - 通常Untextured MeshはAssembly ConfigのMesh定義から生成する。
 * - Debug AABB Meshは通常Untextured Meshと1対1で生成する。
 * - Debug AABB Meshは対応する通常Meshと同じCoordinate IDを保持する。
 * - Debug Axis Meshは各CoordinateについてX / Y / Zの3つを生成する。
 * - Debug Axis Mesh配列はX、Y、Zの順に構築し、
 *   各軸内ではAssembly ConfigのCoordinate定義順に配置する。
 * - 同じ軸に対応するDebug Axis MeshはGeometryとMaterial Resourceを共有する。
 *
 * Ownership / Resource Management:
 * - Resource MapはResource Map Name、初期Transform Tree、
 *   3種類のMesh配列とCoordinate ID配列のStorageを所有する。
 * - Meshに格納するGeometry IDおよびMaterial IDは、
 *   Renderer Frontendが管理するResourceへの識別子である。
 * - Resource MapはImportしたMesh ResourceについてRelease責務を持つが、
 *   Renderer FrontendおよびRegistry自身は所有しない。
 * - 初期Transform TreeはResource Mapが所有し、
 *   Model個体へはCloneした独立のTreeを提供する。
 *
 * Construction / Destruction:
 * - Create中の未完成ResourceはLocal Temporary Variableで管理する。
 * - Mesh群の構築ではImport成功数を追跡し、
 *   Recoverable Failure時にはImport済みResourceのみをReleaseする。
 * - Resource Mapの全Fieldが完成するまでCallerへ公開しない。
 * - 完成したResult Candidateを確認した後、OwnershipをCallerへ移転する。
 * - Destroyでは各Mesh群のRelease後に、初期Transform Tree、
 *   Resource Map NameおよびResource Map自身のStorageを解放する。
 * - Destroy途中でReleaseに失敗した場合、
 *   Resource MapがCanonical Stateを維持する保証はない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

// ============================================================
// Private Type Definitions
// ============================================================
struct untextured_model_resource_map {
    choco_string_t* resource_map_name;

    // 個体生成用の初期Transform Tree
    model_transform_tree_t* transform_tree;

    // Untextured Mesh
    size_t untextured_mesh_count;
    untextured_mesh_t* untextured_meshes;
    uint16_t* untextured_mesh_coordinate_ids;

    // Debug表示用AABB Line Mesh
    size_t debug_aabb_mesh_count;
    line_mesh_t* debug_aabb_meshes;
    uint16_t* debug_aabb_mesh_coordinate_ids;

    // Debug表示用Untextured Mesh Axis
    size_t axis_mesh_count;
    untextured_mesh_t* axis_meshes;
    uint16_t* axis_mesh_coordinate_ids;
};

// ============================================================
// Private Function Declarations
// ============================================================
// Untextured Mesh Helpers
static model_framework_result_t untextured_meshes_create(renderer_frontend_t* renderer_frontend_, const untextured_model_assembly_config_t* config_, const char* asset_root_path_, size_t* out_mesh_count_, untextured_mesh_t** out_meshes_, uint16_t** out_coordinate_ids_);
static model_framework_result_t untextured_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, untextured_mesh_t** meshes_, uint16_t** coordinate_ids_);

// Debug AABB Mesh Helpers
static model_framework_result_t debug_aabb_meshes_create(renderer_frontend_t* renderer_frontend_, const untextured_model_assembly_config_t* config_, const untextured_mesh_t* untextured_meshes_, const uint16_t* untextured_mesh_coordinate_ids_, vec4u8_t aabb_color_, size_t* out_mesh_count_, line_mesh_t** out_meshes_, uint16_t** out_coordinate_ids_);
static model_framework_result_t debug_aabb_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, line_mesh_t** meshes_, uint16_t** coordinate_ids_);
static model_framework_result_t debug_aabb_mesh_resource_name_create(const char* source_resource_name_, choco_string_t** out_string_);

// Debug Axis Mesh Helpers
static model_framework_result_t debug_axis_meshes_create(renderer_frontend_t* renderer_frontend_, const char* resource_map_name_, const untextured_model_assembly_config_t* config_, const cone_t* debug_axis_cone_, const cylinder_t* debug_axis_cylinder_, uint16_t debug_axis_segment_count_, size_t* out_mesh_count_, untextured_mesh_t** out_meshes_, uint16_t** out_coordinate_ids_);
static model_framework_result_t debug_axis_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, untextured_mesh_t** meshes_, uint16_t** coordinate_ids_);
static model_framework_result_t debug_axis_vertices_create(vec3f_t rotation_, const cone_t* cone_, const cylinder_t* cylinder_, uint16_t segment_count_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_);
static model_framework_result_t debug_axis_mesh_resource_name_create(const char* resource_map_name_, const char* resource_category_, const char* axis_name_, choco_string_t** out_string_);

// Transform Tree Helpers
static model_framework_result_t transform_tree_create_from_config(const untextured_model_assembly_config_t* config_, model_transform_tree_t** out_transform_tree_);

// ============================================================
// Public API
// ============================================================
/*
 * Validation Policy:
 * - 必須Pointer、Resource Map Nameの非空条件、Debug Axis Segment Countの
 *   下限をBuild Modeによらず検証する。
 * - DEBUG / TESTでは入力Assembly ConfigのCanonical Validityを検証する。
 * - RELEASEではAssembly Configの内部Semanticを成立済みの入力契約として扱う。
 * - Cone / CylinderのSemantic Validationは、それらを直接Consumeする
 *   debug_axis_vertices_create()に委譲する。
 * - DEBUG / TESTでは、完成したResource MapをCallerへ公開する前に
 *   Canonical Result Validationを行う。
 * - Result Validation失敗は内部構築結果の異常としてDATA_CORRUPTEDとする。
 */
model_framework_result_t untextured_model_resource_map_create(
    renderer_frontend_t* renderer_frontend_,
    const char* resource_map_name_,
    const untextured_model_assembly_config_t* config_,
    const char* asset_root_path_,
    vec4u8_t debug_aabb_color_,
    const cone_t* debug_axis_cone_,
    const cylinder_t* debug_axis_cylinder_,
    uint16_t debug_axis_segment_count_,
    untextured_model_resource_map_t** out_resource_map_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    model_framework_result_t ret_cleanup = MODEL_FRAMEWORK_INVALID_ARGUMENT;
    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    untextured_model_resource_map_t* tmp_resource_map = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;
    line_mesh_t* tmp_debug_aabb_meshes = NULL;
    untextured_mesh_t* tmp_untextured_meshes = NULL;
    untextured_mesh_t* tmp_axis_meshes = NULL;

    uint16_t* tmp_debug_aabb_mesh_coordinate_ids = NULL;
    uint16_t* tmp_untextured_mesh_coordinate_ids = NULL;
    uint16_t* tmp_axis_mesh_coordinate_ids = NULL;

    choco_string_t* tmp_resource_map_name = NULL;

    size_t tmp_untextured_mesh_count = 0;
    size_t tmp_debug_aabb_mesh_count = 0;
    size_t tmp_axis_mesh_count = 0;

    // Preconditions.
    // NOTE: Debug Axis生成無効時はsegment_count_ = 0, debug_axis_cone_ = NULL, debug_axis_cylinder_ = NULLを許容するように変更しても良い, 当面は必須にする
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_name_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "resource_map_name_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(debug_axis_cone_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "debug_axis_cone_")
    IF_ARG_NULL_GOTO_CLEANUP(debug_axis_cylinder_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "debug_axis_cylinder_")
    IF_ARG_NULL_GOTO_CLEANUP(out_resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_create", "out_resource_map_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_resource_map_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "untextured_model_resource_map_create", "*out_resource_map_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_model_assembly_config_is_valid(config_)) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - Provided config_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
#endif
    if('\0' == resource_map_name_[0]) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - Provided resource_map_name_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(3 > debug_axis_segment_count_) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - Provided debug_axis_segment_count_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Prepare.
    ret_general_allocator = general_allocator_allocate(sizeof(untextured_model_resource_map_t), GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_resource_map);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        if(GENERAL_ALLOCATOR_INVALID_ARGUMENT == ret_general_allocator) {
            ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_create_from_c_string(resource_map_name_, &tmp_resource_map_name);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        if(CHOCO_STRING_INVALID_ARGUMENT == ret_choco_string) {
            ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
        }
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - choco_string_create_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = untextured_meshes_create(renderer_frontend_, config_, asset_root_path_, &tmp_untextured_mesh_count, &tmp_untextured_meshes, &tmp_untextured_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - untextured_meshes_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = debug_aabb_meshes_create(renderer_frontend_, config_, tmp_untextured_meshes, tmp_untextured_mesh_coordinate_ids, debug_aabb_color_, &tmp_debug_aabb_mesh_count, &tmp_debug_aabb_meshes, &tmp_debug_aabb_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - debug_aabb_meshes_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = debug_axis_meshes_create(renderer_frontend_, resource_map_name_, config_, debug_axis_cone_, debug_axis_cylinder_, debug_axis_segment_count_, &tmp_axis_mesh_count, &tmp_axis_meshes, &tmp_axis_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - debug_axis_meshes_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = transform_tree_create_from_config(config_, &tmp_transform_tree);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - transform_tree_create_from_config failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    tmp_resource_map->resource_map_name = tmp_resource_map_name;

    tmp_resource_map->debug_aabb_mesh_coordinate_ids = tmp_debug_aabb_mesh_coordinate_ids;
    tmp_resource_map->debug_aabb_mesh_count = tmp_debug_aabb_mesh_count;
    tmp_resource_map->debug_aabb_meshes = tmp_debug_aabb_meshes;

    tmp_resource_map->untextured_mesh_coordinate_ids = tmp_untextured_mesh_coordinate_ids;
    tmp_resource_map->untextured_mesh_count = tmp_untextured_mesh_count;
    tmp_resource_map->untextured_meshes = tmp_untextured_meshes;

    tmp_resource_map->axis_mesh_coordinate_ids = tmp_axis_mesh_coordinate_ids;
    tmp_resource_map->axis_mesh_count = tmp_axis_mesh_count;
    tmp_resource_map->axis_meshes = tmp_axis_meshes;

    tmp_resource_map->transform_tree = tmp_transform_tree;

    // Result Validation.
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_model_resource_map_is_valid(tmp_resource_map)) {
        ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_model_resource_map_create(%s) - Result validation failed for 'tmp_resource_map'.", model_framework_result_to_str(ret));
        goto cleanup;
    }
#endif

    // Output.
    *out_resource_map_ = tmp_resource_map;

    tmp_resource_map_name = NULL;
    tmp_transform_tree = NULL;
    tmp_debug_aabb_meshes = NULL;
    tmp_debug_aabb_mesh_coordinate_ids = NULL;
    tmp_untextured_meshes = NULL;
    tmp_untextured_mesh_coordinate_ids = NULL;
    tmp_axis_meshes = NULL;
    tmp_axis_mesh_coordinate_ids = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }

    if(NULL != tmp_debug_aabb_mesh_coordinate_ids && NULL != tmp_debug_aabb_meshes) {
        ret_cleanup = debug_aabb_meshes_destroy(renderer_frontend_, tmp_debug_aabb_mesh_count, &tmp_debug_aabb_meshes, &tmp_debug_aabb_mesh_coordinate_ids);
        if(MODEL_FRAMEWORK_SUCCESS != ret_cleanup) {
            ret_cleanup = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_model_resource_map_create(%s) - debug_aabb_meshes_destroy failed.", model_framework_result_to_str(ret_cleanup));
            return ret_cleanup;
        }
    }
    if(NULL != tmp_untextured_mesh_coordinate_ids && NULL != tmp_untextured_meshes) {
        ret_cleanup = untextured_meshes_destroy(renderer_frontend_, tmp_untextured_mesh_count, &tmp_untextured_meshes, &tmp_untextured_mesh_coordinate_ids);
        if(MODEL_FRAMEWORK_SUCCESS != ret_cleanup) {
            ret_cleanup = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_model_resource_map_create(%s) - untextured_meshes_destroy failed.", model_framework_result_to_str(ret_cleanup));
            return ret_cleanup;
        }
    }
    if(NULL != tmp_axis_mesh_coordinate_ids && NULL != tmp_axis_meshes) {
        ret_cleanup = debug_axis_meshes_destroy(renderer_frontend_, tmp_axis_mesh_count, &tmp_axis_meshes, &tmp_axis_mesh_coordinate_ids);
        if(MODEL_FRAMEWORK_SUCCESS != ret_cleanup) {
            ret_cleanup = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_model_resource_map_create(%s) - debug_axis_meshes_destroy failed.", model_framework_result_to_str(ret_cleanup));
            return ret_cleanup;
        }
    }
    if(NULL != tmp_transform_tree) {
        model_transform_tree_destroy(&tmp_transform_tree);
    }
    if(NULL != tmp_resource_map_name) {
        choco_string_destroy(&tmp_resource_map_name);
    }
    if(NULL != tmp_resource_map) {
        general_allocator_free((void**)&tmp_resource_map, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerをBuild Modeによらず検証する。
 * - DEBUG / TESTではOwned ResourceのRelease前に
 *   Resource MapのCanonical Validityを検証する。
 * - Canonical Validation失敗は成立済み内部状態の破損として
 *   DATA_CORRUPTEDとし、Destroyを開始しない。
 * - 各Mesh ResourceのReleaseに必要なSemantic Validationは
 *   Renderer Frontendへ委譲する。
 * - Logical Lifetime終了後のPostcondition Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_destroy(renderer_frontend_t* renderer_frontend_, untextured_model_resource_map_t** resource_map_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_destroy", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(*resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_destroy", "*resource_map_")
#if defined(DEBUG_BUILD) || defined(TEST_BUILD)
    if(!untextured_model_resource_map_is_valid(*resource_map_)) {
        ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
        ERROR_MESSAGE("untextured_model_resource_map_destroy(%s) - Precondition validation failed for 'resource_map_'.", model_framework_result_to_str(ret));
        goto cleanup;
    }
#endif

    // debug axis mesh
    ret = debug_axis_meshes_destroy(renderer_frontend_, (*resource_map_)->axis_mesh_count, &(*resource_map_)->axis_meshes, &(*resource_map_)->axis_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_destroy(%s) - debug_axis_meshes_destroy failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // debug line mesh
    ret = debug_aabb_meshes_destroy(renderer_frontend_, (*resource_map_)->debug_aabb_mesh_count, &(*resource_map_)->debug_aabb_meshes, &(*resource_map_)->debug_aabb_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_destroy(%s) - debug_aabb_meshes_destroy failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // untextured mesh
    ret = untextured_meshes_destroy(renderer_frontend_, (*resource_map_)->untextured_mesh_count, &(*resource_map_)->untextured_meshes, &(*resource_map_)->untextured_mesh_coordinate_ids);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_destroy(%s) - untextured_meshes_destroy failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    if(NULL != (*resource_map_)->transform_tree) {
        model_transform_tree_destroy(&(*resource_map_)->transform_tree);
    }

    choco_string_destroy(&(*resource_map_)->resource_map_name);
    general_allocator_free((void**)resource_map_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerとOutput先の未使用状態をBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 * - Clone対象となるTransform TreeのSemantic Validationは
 *   model_transform_tree_clone()へ委譲する。
 * - Resource Map自身のStateを変更しないため、
 *   Postcondition Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_transform_tree_clone(const untextured_model_resource_map_t* resource_map_, model_transform_tree_t** out_transform_tree_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_transform_tree_clone", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_transform_tree_clone", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "untextured_model_resource_map_transform_tree_clone", "*out_transform_tree_")

    ret = model_transform_tree_clone(resource_map_->transform_tree, out_transform_tree_);
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("untextured_model_resource_map_transform_tree_clone(%s) - model_transform_tree_clone failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerとMesh IndexのRangeをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 * - Geometry ID、Material IDのRegistry上のSemantic Validityは検証しない。
 * - Read-only OperationのためPostcondition Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_untextured_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_, uint16_t* out_coordinate_id_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_untextured_mesh_binding_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_untextured_mesh_binding_get", "out_mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_id_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_untextured_mesh_binding_get", "out_coordinate_id_")
    if(mesh_index_ >= resource_map_->untextured_mesh_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_untextured_mesh_binding_get(%s) - Provided mesh_index_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = resource_map_->untextured_meshes[mesh_index_].geometry_id;
    out_mesh_->material_id = resource_map_->untextured_meshes[mesh_index_].material_id;
    *out_coordinate_id_ = resource_map_->untextured_mesh_coordinate_ids[mesh_index_];

    return MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerのみをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_untextured_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_untextured_mesh_count_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_untextured_mesh_count_get", "out_count_")

    *out_count_ = resource_map_->untextured_mesh_count;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerとMesh IndexのRangeをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 * - Geometry IDのRegistry上のSemantic Validityは検証しない。
 * - Read-only OperationのためPostcondition Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_debug_aabb_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, line_mesh_t* out_mesh_, uint16_t* out_coordinate_id_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_aabb_mesh_binding_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_aabb_mesh_binding_get", "out_mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_id_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_aabb_mesh_binding_get", "out_coordinate_id_")
    if(mesh_index_ >= resource_map_->debug_aabb_mesh_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_debug_aabb_mesh_binding_get(%s) - Provided mesh_index_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = resource_map_->debug_aabb_meshes[mesh_index_].geometry_id;
    *out_coordinate_id_ = resource_map_->debug_aabb_mesh_coordinate_ids[mesh_index_];

    return MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerのみをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_debug_aabb_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_aabb_mesh_count_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_aabb_mesh_count_get", "out_count_")

    *out_count_ = resource_map_->debug_aabb_mesh_count;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerとMesh IndexのRangeをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 * - Geometry ID、Material IDのRegistry上のSemantic Validityは検証しない。
 * - Read-only OperationのためPostcondition Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_debug_axis_mesh_binding_get(const untextured_model_resource_map_t* resource_map_, uint16_t mesh_index_, untextured_mesh_t* out_mesh_, uint16_t* out_coordinate_id_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_axis_mesh_binding_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_axis_mesh_binding_get", "out_mesh_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_id_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_axis_mesh_binding_get", "out_coordinate_id_")
    if(mesh_index_ >= resource_map_->axis_mesh_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_model_resource_map_debug_axis_mesh_binding_get(%s) - Provided axis mesh index is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    out_mesh_->geometry_id = resource_map_->axis_meshes[mesh_index_].geometry_id;
    out_mesh_->material_id = resource_map_->axis_meshes[mesh_index_].material_id;
    *out_coordinate_id_ = resource_map_->axis_mesh_coordinate_ids[mesh_index_];

    return MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 必須PointerのみをBuild Modeによらず検証する。
 * - Resource Map全体のCanonical Validationは行わない。
 */
model_framework_result_t untextured_model_resource_map_debug_axis_mesh_count_get(const untextured_model_resource_map_t* resource_map_, size_t* out_count_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_axis_mesh_count_get", "resource_map_")
    IF_ARG_NULL_GOTO_CLEANUP(out_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_model_resource_map_debug_axis_mesh_count_get", "out_count_")

    *out_count_ = resource_map_->axis_mesh_count;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

/*
 * Validation Policy:
 * - 本APIはResource MapのCanonical Validatorであり、
 *   Build Modeによらず同一のValidation Semanticsを持つ。
 * - Root PointerについてはNULLのみを検証し、
 *   Root自身のAllocation ValidityはCallerの責務とする。
 * - Owned StringとTransform TreeについてはAllocation Validityを確認した後、
 *   各Owner ModuleのCanonical Validatorへ委譲する。
 * - Mesh配列とCoordinate ID配列については、Countの整合性、
 *   Allocation Sizeの表現可能性、PointerのAllocation Validityを検証する。
 * - Mesh配列のActual Allocation Sizeや各ElementのRegistry登録状態、
 *   Coordinate IDとTransform Treeの参照整合性は検証対象としない。
 */
bool untextured_model_resource_map_is_valid(const untextured_model_resource_map_t* resource_map_) {
    if(NULL == resource_map_) {
        return false;
    }

    // Resource Map Name.
    if(NULL == resource_map_->resource_map_name) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(resource_map_->resource_map_name)) {
        return false;
    }
    if(!choco_string_is_valid(resource_map_->resource_map_name)) {
        return false;
    }
    if(0 == choco_string_length(resource_map_->resource_map_name)) {
        return false;
    }

    // Transform Tree.
    if(NULL == resource_map_->transform_tree) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(resource_map_->transform_tree)) {
        return false;
    }
    if(!model_transform_tree_is_valid(resource_map_->transform_tree)) {
        return false;
    }

    // Mesh Count.
    if(0 == resource_map_->untextured_mesh_count) {
        return false;
    }
    if(resource_map_->debug_aabb_mesh_count != resource_map_->untextured_mesh_count) {
        return false;
    }
    if(0 == resource_map_->axis_mesh_count || 0 != resource_map_->axis_mesh_count % 3) {
        return false;
    }

    // Allocation size representability.
    if(resource_map_->untextured_mesh_count > SIZE_MAX / sizeof(untextured_mesh_t) || resource_map_->untextured_mesh_count > SIZE_MAX / sizeof(uint16_t)) {
        return false;
    }
    if(resource_map_->debug_aabb_mesh_count > SIZE_MAX / sizeof(line_mesh_t) || resource_map_->debug_aabb_mesh_count > SIZE_MAX / sizeof(uint16_t)) {
        return false;
    }
    if(resource_map_->axis_mesh_count > SIZE_MAX / sizeof(untextured_mesh_t) || resource_map_->axis_mesh_count > SIZE_MAX / sizeof(uint16_t)) {
        return false;
    }

    // Untextured Mesh.
    if(NULL == resource_map_->untextured_meshes || NULL == resource_map_->untextured_mesh_coordinate_ids) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(resource_map_->untextured_meshes) || !general_allocator_ptr_is_allocated(resource_map_->untextured_mesh_coordinate_ids)) {
        return false;
    }

    // Debug AABB Mesh.
    if(NULL == resource_map_->debug_aabb_meshes || NULL == resource_map_->debug_aabb_mesh_coordinate_ids) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(resource_map_->debug_aabb_meshes) || !general_allocator_ptr_is_allocated(resource_map_->debug_aabb_mesh_coordinate_ids)) {
        return false;
    }

    // Untextured MeshとDebug AABB Meshの整合性
#if ENABLE_VALID_DEEP == 1
    for(size_t i = 0; i != resource_map_->untextured_mesh_count; ++i) {
        if(resource_map_->untextured_mesh_coordinate_ids[i] != resource_map_->debug_aabb_mesh_coordinate_ids[i]) {
            return false;
        }
    }
#endif

    // Debug Axis Mesh.
    if(NULL == resource_map_->axis_meshes || NULL == resource_map_->axis_mesh_coordinate_ids) {
        return false;
    }
    if(!general_allocator_ptr_is_allocated(resource_map_->axis_meshes) || !general_allocator_ptr_is_allocated(resource_map_->axis_mesh_coordinate_ids)) {
        return false;
    }

    return true;
}

// ============================================================
// Untextured Mesh Helpers
// ============================================================
static model_framework_result_t untextured_meshes_create(renderer_frontend_t* renderer_frontend_, const untextured_model_assembly_config_t* config_, const char* asset_root_path_, size_t* out_mesh_count_, untextured_mesh_t** out_meshes_, uint16_t** out_coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    untextured_mesh_t* tmp_meshes = NULL;
    uint16_t* tmp_coordinate_ids = NULL;

    size_t allocation_size = 0;
    size_t imported_mesh_count = 0;
    untextured_mesh_t tmp_mesh = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(asset_root_path_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_create", "asset_root_path_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_create", "out_mesh_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_create", "out_meshes_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_meshes_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "untextured_meshes_create", "*out_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_create", "out_coordinate_ids_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_coordinate_ids_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "untextured_meshes_create", "*out_coordinate_ids_")
    if(0 == config_->mesh_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("untextured_meshes_create(%s) - Provided config_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // mesh配列リソース確保
    if(!size_mul_checked(sizeof(untextured_mesh_t), config_->mesh_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("untextured_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_meshes);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // coordinate配列リソース確保
    if(!size_mul_checked(sizeof(uint16_t), config_->mesh_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("untextured_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("untextured_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != config_->mesh_count; ++i) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_import(renderer_frontend_, &config_->mesh_configs[i], asset_root_path_, &tmp_mesh);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = model_framework_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("untextured_meshes_create(%s) - renderer_frontend_untextured_mesh_import failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        tmp_meshes[i].geometry_id = tmp_mesh.geometry_id;
        tmp_meshes[i].material_id = tmp_mesh.material_id;

        tmp_coordinate_ids[i] = config_->mesh_coordinate_ids[i];

        imported_mesh_count++;
    }

    // Output.
    *out_coordinate_ids_ = tmp_coordinate_ids;
    *out_meshes_ = tmp_meshes;
    *out_mesh_count_ = config_->mesh_count;

    tmp_meshes = NULL;
    tmp_coordinate_ids = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_meshes) {
        for(size_t i = 0; i != imported_mesh_count; ++i) {
            if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_meshes[i])) {
                ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
                ERROR_MESSAGE("untextured_meshes_create(%s) - renderer_frontend_untextured_mesh_release failed.", model_framework_result_to_str(ret));
                return ret;
            }
        }
    }
    if(NULL != tmp_coordinate_ids) {
        general_allocator_free((void**)&tmp_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_meshes) {
        general_allocator_free((void**)&tmp_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static model_framework_result_t untextured_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, untextured_mesh_t** meshes_, uint16_t** coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_destroy", "meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(*meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_destroy", "*meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_destroy", "coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(*coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "untextured_meshes_destroy", "*coordinate_ids_")

    // untextured mesh
    for(size_t i = 0; i != mesh_count_; ++i) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &(*meshes_)[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_meshes_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }
    }
    general_allocator_free((void**)coordinate_ids_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)meshes_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

// ============================================================
// Debug AABB Mesh Helpers
// ============================================================
static model_framework_result_t debug_aabb_meshes_create(renderer_frontend_t* renderer_frontend_, const untextured_model_assembly_config_t* config_, const untextured_mesh_t* untextured_meshes_, const uint16_t* untextured_mesh_coordinate_ids_, vec4u8_t aabb_color_, size_t* out_mesh_count_, line_mesh_t** out_meshes_, uint16_t** out_coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    line_mesh_t* tmp_meshes = NULL;
    choco_string_t* tmp_resource_name = NULL;
    uint16_t* tmp_coordinate_ids = NULL;

    size_t allocation_size = 0;
    size_t imported_mesh_count = 0;
    aabb_3d_t tmp_aabb = { 0 };

    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(untextured_meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "untextured_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(untextured_mesh_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "untextured_mesh_coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "out_mesh_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "out_meshes_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_meshes_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_aabb_meshes_create", "*out_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_create", "out_coordinate_ids_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_coordinate_ids_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_aabb_meshes_create", "*out_coordinate_ids_")
    if(0 == config_->mesh_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("debug_aabb_meshes_create(%s) - Provided config_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    if(!size_mul_checked(sizeof(line_mesh_t), config_->mesh_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_aabb_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_meshes);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("debug_aabb_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    if(!size_mul_checked(sizeof(uint16_t), config_->mesh_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_aabb_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("debug_aabb_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    for(size_t i = 0; i != config_->mesh_count; ++i) {
        ret = debug_aabb_mesh_resource_name_create(config_->mesh_configs[i].geometry_resource_name, &tmp_resource_name);
        if(MODEL_FRAMEWORK_SUCCESS != ret) {
            ERROR_MESSAGE("debug_aabb_meshes_create(%s) - debug_aabb_mesh_resource_name_create failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        // Untextured MeshからLocal AABBを取得
        ret_renderer_frontend = renderer_frontend_untextured_mesh_local_aabb_get(renderer_frontend_, &untextured_meshes_[i], &tmp_aabb);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = model_framework_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("debug_aabb_meshes_create(%s) - renderer_frontend_untextured_mesh_local_aabb_get failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        // AABBをLine MeshとしてImport
        ret_renderer_frontend = renderer_frontend_line_mesh_import_from_aabb(renderer_frontend_, choco_string_c_str(tmp_resource_name), &tmp_aabb, aabb_color_, &tmp_meshes[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = model_framework_result_convert_renderer_frontend(ret_renderer_frontend);
            ERROR_MESSAGE("debug_aabb_meshes_create(%s) - renderer_frontend_line_mesh_import_from_aabb failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        choco_string_destroy(&tmp_resource_name);
        imported_mesh_count++;

        // 元Meshと同じCoordinate IDを使用
        tmp_coordinate_ids[i] = untextured_mesh_coordinate_ids_[i];
    }

    // Output.
    *out_meshes_ = tmp_meshes;
    *out_coordinate_ids_ = tmp_coordinate_ids;
    *out_mesh_count_ = config_->mesh_count;

    tmp_meshes = NULL;
    tmp_resource_name = NULL;
    tmp_coordinate_ids = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_resource_name) {
        choco_string_destroy(&tmp_resource_name);
    }

    if(NULL != tmp_meshes) {
        for(size_t i = 0; i != imported_mesh_count; ++i) {
            if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_line_mesh_release(renderer_frontend_, &tmp_meshes[i])) {
                ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
                ERROR_MESSAGE("debug_aabb_meshes_create(%s) - renderer_frontend_line_mesh_release failed.", model_framework_result_to_str(ret));
                return ret;
            }
        }
    }
    if(NULL != tmp_meshes) {
        general_allocator_free((void**)&tmp_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_coordinate_ids) {
        general_allocator_free((void**)&tmp_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static model_framework_result_t debug_aabb_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, line_mesh_t** meshes_, uint16_t** coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_destroy", "meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(*meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_destroy", "*meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_destroy", "coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(*coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_meshes_destroy", "*coordinate_ids_")

    for(size_t i = 0; i != mesh_count_; ++i) {
        ret_renderer_frontend = renderer_frontend_line_mesh_release(renderer_frontend_, &(*meshes_)[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("debug_aabb_meshes_destroy(%s) - renderer_frontend_line_mesh_release failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }
    }
    general_allocator_free((void**)coordinate_ids_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)meshes_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

// source_resource_name_がxxxの場合、xxx_debug_aabbという名称がout_string_に格納される
static model_framework_result_t debug_aabb_mesh_resource_name_create(const char* source_resource_name_, choco_string_t** out_string_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(source_resource_name_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_mesh_resource_name_create", "source_resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_aabb_mesh_resource_name_create", "out_string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_string_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_aabb_mesh_resource_name_create", "*out_string_")

    ret_choco_string = choco_string_create_from_c_string(source_resource_name_, &tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_aabb_mesh_resource_name_create(%s) - choco_string_create_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_debug_aabb", tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_aabb_mesh_resource_name_create(%s) - choco_string_concat_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    *out_string_ = tmp_string;
    tmp_string = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_string) {
        choco_string_destroy(&tmp_string);
    }
    return ret;
}

// ============================================================
// Debug Axis Mesh Helpers
// ============================================================
static model_framework_result_t debug_axis_meshes_create(
    renderer_frontend_t* renderer_frontend_,
    const char* resource_map_name_,
    const untextured_model_assembly_config_t* config_,
    const cone_t* debug_axis_cone_,
    const cylinder_t* debug_axis_cylinder_,
    uint16_t debug_axis_segment_count_,
    size_t* out_mesh_count_,
    untextured_mesh_t** out_meshes_,
    uint16_t** out_coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    untextured_mesh_t* tmp_meshes = NULL;
    uint16_t* tmp_coordinate_ids = NULL;

    size_t allocation_size = 0;
    size_t imported_mesh_count = 0;

    choco_string_t* tmp_geometry_resource_name = NULL;
    choco_string_t* tmp_material_resource_name = NULL;

    size_t tmp_vertex_count = 0;
    point_normal_vertex_t* tmp_vertices = NULL;

    const char* axis_names[3] = { "x", "y", "z" };
    const vec3f_t axis_rotations[3] = { vec3f_initialize(0.0f, 90.0f, 0.0f), vec3f_initialize(-90.0f, 0.0f, 0.0f), vec3f_initialize(0.0f, 0.0f, 0.0f) };
    const untextured_mesh_material_t axis_materials[3] = {
        {
            .diffuse = vec3f_initialize(1.0f, 0.0f, 0.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        },
        {
            .diffuse = vec3f_initialize(0.0f, 1.0f, 0.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        },
        {
            .diffuse = vec3f_initialize(0.0f, 0.0f, 1.0f), .ambient = vec3f_initialize(0.0f, 0.0f, 0.0f), .shininess = 0.0f, .specular = vec3f_initialize(0.0f, 0.0f, 0.0f)
        }
    };

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_map_name_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "resource_map_name_")
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(debug_axis_cone_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "debug_axis_cone_")
    IF_ARG_NULL_GOTO_CLEANUP(debug_axis_cylinder_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "debug_axis_cylinder_")
    IF_ARG_NULL_GOTO_CLEANUP(out_mesh_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "out_mesh_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "out_meshes_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_meshes_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_axis_meshes_create", "*out_meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(out_coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_create", "out_coordinate_ids_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_coordinate_ids_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_axis_meshes_create", "*out_coordinate_ids_")

    // Resource Allocation.
    if(!size_mul_checked(sizeof(untextured_mesh_t) * 3, config_->coordinate_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_axis_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_meshes);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("debug_axis_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    if(!size_mul_checked(sizeof(uint16_t) * 3, config_->coordinate_count, &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_axis_meshes_create(%s) - allocation overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_coordinate_ids);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("debug_axis_meshes_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Generate Geometry, Mesh Import.
    for(size_t i = 0; i != 3; ++i) {    // X, Y, Z各軸ループ
        // Geometry, Materialは共有するので生成は各軸1回でok
        ret = debug_axis_mesh_resource_name_create(resource_map_name_, "geometry", axis_names[i], &tmp_geometry_resource_name);
        if(MODEL_FRAMEWORK_SUCCESS != ret) {
            ERROR_MESSAGE("debug_axis_meshes_create(%s) - debug_axis_mesh_resource_name_create failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        ret = debug_axis_mesh_resource_name_create(resource_map_name_, "material", axis_names[i], &tmp_material_resource_name);
        if(MODEL_FRAMEWORK_SUCCESS != ret) {
            ERROR_MESSAGE("debug_axis_meshes_create(%s) - debug_axis_mesh_resource_name_create failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        ret = debug_axis_vertices_create(axis_rotations[i], debug_axis_cone_, debug_axis_cylinder_, debug_axis_segment_count_, &tmp_vertex_count, &tmp_vertices);
        if(MODEL_FRAMEWORK_SUCCESS != ret) {
            ERROR_MESSAGE("debug_axis_meshes_create(%s) - debug_axis_vertices_create failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        for(size_t j = 0; j != config_->coordinate_count; ++j) {
            ret_renderer_frontend = renderer_frontend_untextured_mesh_import_from_vertices(renderer_frontend_, choco_string_c_str(tmp_geometry_resource_name), choco_string_c_str(tmp_material_resource_name), tmp_vertices, tmp_vertex_count, &axis_materials[i], &tmp_meshes[imported_mesh_count]);
            if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
                ret = model_framework_result_convert_renderer_frontend(ret_renderer_frontend);
                ERROR_MESSAGE("debug_axis_meshes_create(%s) - renderer_frontend_untextured_mesh_import_from_vertices failed.", model_framework_result_to_str(ret));
                goto cleanup;
            }
            tmp_coordinate_ids[imported_mesh_count] = config_->coordinate_configs[j].coordinate_id;
            imported_mesh_count++;
        }

        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
        choco_string_destroy(&tmp_geometry_resource_name);
        choco_string_destroy(&tmp_material_resource_name);
    }

    // Output.
    *out_meshes_ = tmp_meshes;
    *out_coordinate_ids_ = tmp_coordinate_ids;
    *out_mesh_count_ = config_->coordinate_count * 3;

    tmp_meshes = NULL;
    tmp_coordinate_ids = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_meshes) {
        for(size_t i = 0; i != imported_mesh_count; ++i) {
            if(RENDERER_FRONTEND_SUCCESS != renderer_frontend_untextured_mesh_release(renderer_frontend_, &tmp_meshes[i])) {
                ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
                ERROR_MESSAGE("debug_axis_meshes_create(%s) - renderer_frontend_untextured_mesh_release failed.", model_framework_result_to_str(ret));
                return ret;
            }
        }
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }

    choco_string_destroy(&tmp_geometry_resource_name);
    choco_string_destroy(&tmp_material_resource_name);
    if(NULL != tmp_meshes) {
        general_allocator_free((void**)&tmp_meshes, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_coordinate_ids) {
        general_allocator_free((void**)&tmp_coordinate_ids, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

static model_framework_result_t debug_axis_meshes_destroy(renderer_frontend_t* renderer_frontend_, size_t mesh_count_, untextured_mesh_t** meshes_, uint16_t** coordinate_ids_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    renderer_frontend_result_t ret_renderer_frontend = RENDERER_FRONTEND_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(renderer_frontend_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_destroy", "renderer_frontend_")
    IF_ARG_NULL_GOTO_CLEANUP(meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_destroy", "meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(*meshes_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_destroy", "*meshes_")
    IF_ARG_NULL_GOTO_CLEANUP(coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_destroy", "coordinate_ids_")
    IF_ARG_NULL_GOTO_CLEANUP(*coordinate_ids_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_meshes_destroy", "*coordinate_ids_")

    for(size_t i = 0; i != mesh_count_; ++i) {
        ret_renderer_frontend = renderer_frontend_untextured_mesh_release(renderer_frontend_, &(*meshes_)[i]);
        if(RENDERER_FRONTEND_SUCCESS != ret_renderer_frontend) {
            ret = MODEL_FRAMEWORK_DATA_CORRUPTED;
            ERROR_MESSAGE("debug_axis_meshes_destroy(%s) - renderer_frontend_untextured_mesh_release failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }
    }
    general_allocator_free((void**)coordinate_ids_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    general_allocator_free((void**)meshes_, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    return ret;
}

static model_framework_result_t debug_axis_vertices_create(vec3f_t rotation_, const cone_t* cone_, const cylinder_t* cylinder_, uint16_t segment_count_, size_t* out_vertex_count_, point_normal_vertex_t** out_vertices_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    general_allocator_result_t ret_general_allocator = GENERAL_ALLOCATOR_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;

    point_normal_vertex_t* tmp_vertices = NULL;
    point_normal_vertex_t* tmp_cylinder_vertices = NULL;
    point_normal_vertex_t* tmp_cone_vertices = NULL;

    size_t allocation_size = 0;
    size_t tmp_vertex_count = 0;
    size_t tmp_cylinder_vertex_count = 0;
    size_t tmp_cone_vertex_count = 0;
    size_t index = 0;
    mat4x4f_t model_mat = { 0 };

    bool cylinder_generated = false;
    bool cone_generated = false;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(cone_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_vertices_create", "cone_")
    IF_ARG_NULL_GOTO_CLEANUP(cylinder_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_vertices_create", "cylinder_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertex_count_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_vertices_create", "out_vertex_count_")
    IF_ARG_NULL_GOTO_CLEANUP(out_vertices_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_vertices_create", "out_vertices_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_vertices_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_vertices_create", "*out_vertices_")
    if(!cone_is_valid(cone_)) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - Provided cone_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(!cylinder_is_valid(cylinder_)) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - Provided cylinder_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(!vec3f_is_finite(rotation_)) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - Provided rotation_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Generate Geometry.
    ret_geometry = untextured_cylinder_geometry_vertices_create(cylinder_, segment_count_, &tmp_cylinder_vertices, &tmp_cylinder_vertex_count);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = model_framework_result_convert_geometry(ret_geometry);
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - untextured_cylinder_geometry_vertices_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    cylinder_generated = true;

    ret_geometry = untextured_cone_geometry_vertices_create(cone_, segment_count_, &tmp_cone_vertices, &tmp_cone_vertex_count);
    if(GEOMETRY_SUCCESS != ret_geometry) {
        ret = model_framework_result_convert_geometry(ret_geometry);
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - untextured_cone_geometry_vertices_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    cone_generated = true;

    // ConeをCylinderの上面に移動
    mat4f_translation(vec3f_initialize(0.0f, 0.0f, cylinder_->height), &model_mat);
    if(!point_normal_vertices_transform(tmp_cone_vertices, tmp_cone_vertex_count, &model_mat)) {
        ret = MODEL_FRAMEWORK_RUNTIME_ERROR;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - point_normal_vertices_transform failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Cylinder, Coneを1つのGeometryにするため両方の頂点情報を格納する配列リソース確保
    if(!size_add_checked(tmp_cone_vertex_count, tmp_cylinder_vertex_count, &tmp_vertex_count)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - vertex count overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    if(!size_mul_checked(tmp_vertex_count, sizeof(point_normal_vertex_t), &allocation_size)) {
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - allocation size overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_general_allocator = general_allocator_allocate(allocation_size, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER, (void**)&tmp_vertices);
    if(GENERAL_ALLOCATOR_SUCCESS != ret_general_allocator) {
        ret = model_framework_result_convert_general_allocator(ret_general_allocator);
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - general_allocator_allocate failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // 頂点情報コピー
    for(size_t i = 0; i != tmp_cylinder_vertex_count; ++i) {
        tmp_vertices[index] = tmp_cylinder_vertices[i];
        index++;
    }
    for(size_t i = 0; i != tmp_cone_vertex_count; ++i) {
        tmp_vertices[index] = tmp_cone_vertices[i];
        index++;
    }

    // 各軸用に回転
    mat4f_rot_xyz(CHOCO_DEG_TO_RAD(rotation_.elem[0]), CHOCO_DEG_TO_RAD(rotation_.elem[1]), CHOCO_DEG_TO_RAD(rotation_.elem[2]), &model_mat);
    if(!point_normal_vertices_transform(tmp_vertices, tmp_vertex_count, &model_mat)) {
        ret = MODEL_FRAMEWORK_RUNTIME_ERROR;
        ERROR_MESSAGE("debug_axis_vertices_create(%s) - point_normal_vertices_transform failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    *out_vertex_count_ = tmp_vertex_count;
    *out_vertices_ = tmp_vertices;
    tmp_vertices = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(cylinder_generated) {
        general_allocator_free((void**)&tmp_cylinder_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(cone_generated) {
        general_allocator_free((void**)&tmp_cone_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    if(NULL != tmp_vertices) {
        general_allocator_free((void**)&tmp_vertices, GENERAL_ALLOCATOR_MEMORY_TAG_RENDERER);
    }
    return ret;
}

// NOTE:
// - *out_string_ = resource_map_name_ + "_debug_axis_" + resource_category_ + "_" + axis_name_
// - resource_category_ = material or geometry
// - axis_name_ = x or y or z
static model_framework_result_t debug_axis_mesh_resource_name_create(const char* resource_map_name_, const char* resource_category_, const char* axis_name_, choco_string_t** out_string_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    choco_string_result_t ret_choco_string = CHOCO_STRING_INVALID_ARGUMENT;

    choco_string_t* tmp_string = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(resource_map_name_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_mesh_resource_name_create", "resource_map_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_category_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_mesh_resource_name_create", "resource_category_")
    IF_ARG_NULL_GOTO_CLEANUP(axis_name_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_mesh_resource_name_create", "axis_name_")
    IF_ARG_NULL_GOTO_CLEANUP(out_string_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "debug_axis_mesh_resource_name_create", "out_string_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_string_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "debug_axis_mesh_resource_name_create", "*out_string_")

    ret_choco_string = choco_string_create_from_c_string(resource_map_name_, &tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_mesh_resource_name_create(%s) - choco_string_create_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_debug_axis_", tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_mesh_resource_name_create(%s) - choco_string_concat_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string(resource_category_, tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_mesh_resource_name_create(%s) - choco_string_concat_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string("_", tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_mesh_resource_name_create(%s) - choco_string_concat_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret_choco_string = choco_string_concat_from_c_string(axis_name_, tmp_string);
    if(CHOCO_STRING_SUCCESS != ret_choco_string) {
        ret = model_framework_result_convert_choco_string(ret_choco_string);
        ERROR_MESSAGE("debug_axis_mesh_resource_name_create(%s) - choco_string_concat_from_c_string failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    *out_string_ = tmp_string;
    tmp_string = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED == ret || MODEL_FRAMEWORK_SUCCESS == ret) {
        return ret;
    }
    if(NULL != tmp_string) {
        choco_string_destroy(&tmp_string);
    }
    return ret;
}

// ============================================================
// Transform Tree Helpers
// ============================================================
static model_framework_result_t transform_tree_create_from_config(const untextured_model_assembly_config_t* config_, model_transform_tree_t** out_transform_tree_) {
    model_framework_result_t ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;

    ring_queue_result_t ret_ring_queue = RING_QUEUE_INVALID_ARGUMENT;

    ring_queue_t* parent_queue = NULL;
    model_transform_tree_t* tmp_transform_tree = NULL;

    size_t queue_capacity = 0;
    size_t added_count = 0;
    uint16_t current_parent = 0;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(config_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "transform_tree_create_from_config", "config_")
    IF_ARG_NULL_GOTO_CLEANUP(out_transform_tree_, ret, MODEL_FRAMEWORK_INVALID_ARGUMENT, model_framework_result_to_str(MODEL_FRAMEWORK_INVALID_ARGUMENT), "transform_tree_create_from_config", "out_transform_tree_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_transform_tree_, ret, MODEL_FRAMEWORK_BAD_OPERATION, model_framework_result_to_str(MODEL_FRAMEWORK_BAD_OPERATION), "transform_tree_create_from_config", "*out_transform_tree_")
    if(config_->coordinate_count > 0 && NULL == config_->coordinate_configs) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Provided config_ is not valid.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    if(!size_add_checked(config_->coordinate_count, 1, &queue_capacity)) {  // RootのID分を追加したQueue容量
        ret = MODEL_FRAMEWORK_OVERFLOW;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Queue capacity overflow.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    ret_ring_queue = ring_queue_create(queue_capacity, sizeof(uint16_t), alignof(uint16_t), &parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = model_framework_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    ret = model_transform_tree_create(0, &tmp_transform_tree);  // Root Coordinate
    if(MODEL_FRAMEWORK_SUCCESS != ret) {
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_create failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Rootを探索Queueに追加
    current_parent = 0;
    ret_ring_queue = ring_queue_push(&current_parent, sizeof(uint16_t), alignof(uint16_t), parent_queue);
    if(RING_QUEUE_SUCCESS != ret_ring_queue) {
        ret = model_framework_result_convert_ring_queue(ret_ring_queue);
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", model_framework_result_to_str(ret));
        goto cleanup;
    }
    while(!ring_queue_is_empty(parent_queue)) {
        ret_ring_queue = ring_queue_pop(sizeof(uint16_t), alignof(uint16_t), parent_queue, &current_parent);
        if(RING_QUEUE_SUCCESS != ret_ring_queue) {
            ret = model_framework_result_convert_ring_queue(ret_ring_queue);
            ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_pop failed.", model_framework_result_to_str(ret));
            goto cleanup;
        }

        // 現在のParentを持つCoordinateを探索
        for(size_t i = 0; i != config_->coordinate_count; ++i) {
            const model_coordinate_config_t* coordinate = &config_->coordinate_configs[i];
            if(current_parent != coordinate->parent_coordinate_id) {
                continue;
            }

            ret = model_transform_tree_add_child(tmp_transform_tree, current_parent, coordinate->coordinate_id, coordinate->translate, coordinate->rotation);
            if(MODEL_FRAMEWORK_SUCCESS != ret) {
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - model_transform_tree_add_child failed.", model_framework_result_to_str(ret));
                goto cleanup;
            }

            ++added_count;

            // 追加したChildを次のParent候補としてQueueへpush
            ret_ring_queue = ring_queue_push(&coordinate->coordinate_id, sizeof(uint16_t), alignof(uint16_t), parent_queue);
            if(RING_QUEUE_SUCCESS != ret_ring_queue) {
                ret = model_framework_result_convert_ring_queue(ret_ring_queue);
                ERROR_MESSAGE("transform_tree_create_from_config(%s) - ring_queue_push failed.", model_framework_result_to_str(ret));
                goto cleanup;
            }
        }
    }

    // 全CoordinateがRootから到達可能か検査
    if(added_count != config_->coordinate_count) {
        ret = MODEL_FRAMEWORK_INVALID_ARGUMENT;
        ERROR_MESSAGE("transform_tree_create_from_config(%s) - Unreachable coordinates detected.", model_framework_result_to_str(ret));
        goto cleanup;
    }

    // Ownership Commit.
    *out_transform_tree_ = tmp_transform_tree;
    tmp_transform_tree = NULL;

    ret = MODEL_FRAMEWORK_SUCCESS;

cleanup:
    if(MODEL_FRAMEWORK_DATA_CORRUPTED != ret) {
        ring_queue_destroy(&parent_queue);
        model_transform_tree_destroy(&tmp_transform_tree);
    }

    return ret;
}
