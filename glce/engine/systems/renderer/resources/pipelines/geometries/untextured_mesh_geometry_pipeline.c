// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

#include "engine/systems/renderer/resources/pipelines/geometries/untextured_mesh_geometry_pipeline.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"
#include "engine/base/choco_math/choco_math.h"

#include "engine/memory/general_allocator/general_allocator.h"

#include "engine/core/geometry_primitive/vertex.h"

#include "engine/systems/renderer/resources/buffer_managers/vbo_manager.h"
#include "engine/systems/renderer/resources/loaders/core/loader_types.h"
#include "engine/systems/renderer/resources/loaders/stl_ascii_loader.h"
#include "engine/systems/renderer/resources/geometries/core/geometry_types.h"
#include "engine/systems/renderer/resources/geometries/untextured_mesh_geometry.h"
#include "engine/systems/renderer/resources/shaders/core/shader_resource_types.h"
#include "engine/systems/renderer/resources/registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resources/registries/geometries/untextured_mesh_geometry_registry.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_types.h"
#include "engine/systems/renderer/resources/pipelines/core/resource_pipeline_err_utils.h"

/*
 * Module Internal Contract
 *
 * Module State:
 * - 本Moduleは永続的な内部状態を保持しない。
 * - VBO ManagerとGeometry Registryはcallerからborrowする。
 * - STLファイルの読み込み、CPU Geometryの生成、
 *   GPU頂点データ転送、Registry登録、および利用権返却を
 *   一連のOperationとして調整する。
 *
 * Resource Construction:
 * - ASCII STLファイルの読み込みと頂点データ生成は
 *   STL ASCII Loaderへ委譲する。
 * - Loaderはpoint_normal_vertex_tの配列を生成し、
 *   そのownershipを本Operationへ移転する。
 * - CPU Geometryの生成はUntextured Mesh Geometry Moduleへ委譲する。
 * - CPU GeometryはLoaderの頂点配列をコピーして生成する。
 * - VBO Allocationの確保と頂点データの転送は
 *   VBO Managerへ委譲する。
 * - Resource Nameの複製と登録状態の管理は
 *   Geometry Registryへ委譲する。
 *
 * Temporary Ownership:
 * - Loader成功後、返された頂点配列を本Operationが所有する。
 * - 頂点配列はGENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRYを使用して
 *   General Allocatorから確保されている。
 * - CPU Geometry生成後、Registry登録までは
 *   本OperationがCPU Geometryを一時的に所有する。
 * - VBO書込成功後、Registry登録までは
 *   本OperationがVBO Allocationの解放責務を持つ。
 * - VBO書込成功状態はvbo_writtenによって管理する。
 * - VBO Allocation Descriptorはtmp_buffer_rangeに保持する。
 *
 * - VBO転送とDraw Range構築後、一時頂点配列を解放する。
 * - 一時頂点配列のownershipはRegistryへ移転しない。
 * - Registry登録成功時、CPU GeometryとVBO Allocationの
 *   解放責務はGeometry Registryへ移転する。
 * - Registry登録成功後、本Operationはこれらを解放しない。
 *
 * Draw Range:
 * - VBO書込サイズはVertex Countと
 *   sizeof(point_normal_vertex_t)から計算する。
 * - size_mul_checked()によって書込サイズのoverflowを検査する。
 * - VBO Allocation Offsetはsizeof(point_normal_vertex_t)の
 *   倍数である必要がある。
 * - Offsetの整合性を確認した後、Draw Rangeを構築する。
 * - first_vertex_countはAllocation Offsetを
 *   sizeof(point_normal_vertex_t)で除算して求める。
 * - vertex_countにはLoaderが返したVertex Countを使用する。
 *
 * Commit:
 * - CPU GeometryとVBO Allocation Descriptorを
 *   Geometry Registryへ渡す。
 * - Registry登録成功がOwnership TransferのCommit境界となる。
 * - Registryは登録ResourceのReference Countを1に初期化する。
 * - Geometry IDは登録成功後にcallerへ出力する。
 *
 * Failure / Rollback:
 * - Commit前の回復可能な失敗では、
 *   確保済みVBO Allocationを先に解放する。
 * - VBO Allocationの解放成功後、CPU Geometryを破棄する。
 * - Loaderから取得した一時頂点配列が残っている場合は、
 *   General Allocatorを使用して解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDとして処理を中断する。
 * - DATA_CORRUPTEDの場合は通常のcleanupを行わない。
 * - 特にRegistry登録時の整合性異常では、
 *   Ownership Transferの完了状態を保証できないため、
 *   Pipeline側からResourceを解放しない。
 * - 回復可能な失敗ではGeometry IDの出力先を変更しない。
 *
 * Validation Responsibility:
 * - 外部ASCII STLファイルの構文と頂点データの
 *   semantic validityはSTL ASCII Loaderへ委譲する。
 * - CPU Geometryの構造的条件と頂点数の妥当性は
 *   Untextured Mesh Geometry Moduleへ委譲する。
 * - VBO Allocationと書込条件の検証はVBO Managerへ委譲する。
 * - Resource Name、登録状態、Reference Countの検証は
 *   Geometry Registryへ委譲する。
 * - 本Moduleは書込サイズのoverflowと、
 *   Allocation Offset / Vertex Strideの整合性を検証する。
 * - 下位Moduleのcanonical validationを重複して実行しない。
 *
 * AI支援:
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - vbo_manager_、geometry_registry_、resource_name_、
 *   resource_fullpath_、out_geometry_id_のNULLを
 *   全Build Modeで検査する。
 * - File Pathの空文字列条件と外部ファイル内容のvalidationは
 *   stl_ascii_loader_load()へ委譲する。
 * - Resource Nameの空文字列条件と重複登録の判定は、
 *   Geometry Registryのregister()へ委譲する。
 * - VBO ManagerとGeometry Registryの内部整合性は、
 *   各ModuleのAPI contractとvalidationに委ねる。
 * - 本Pipelineでは下位Resource全体の
 *   canonical validationを重複して実行しない。
 *
 * STL Loading:
 * - stl_ascii_loader_load()によってASCII STLファイルを読み込む。
 * - Loaderは頂点数とpoint_normal_vertex_t配列を生成する。
 * - Loader成功後、頂点配列のownershipを本Operationが取得する。
 * - Loaderが失敗した場合はResult Codeを変換して返す。
 * - Loader成功後は頂点データのsemantic contractを信頼し、
 *   同じvalidationを繰り返さない。
 *
 * Resource Construction:
 * - untextured_mesh_geometry_create_from_vertices()によって、
 *   Loaderの頂点配列から独立したCPU Geometryを生成する。
 * - 頂点数の正値条件と3の倍数条件は、
 *   Untextured Mesh Geometry Moduleへ委譲する。
 * - size_mul_checked()によってVBO書込サイズのoverflowを検査する。
 * - vbo_manager_write()によってVBO Allocationを確保し、
 *   Loaderの頂点配列をGPUへ転送する。
 * - 書込成功後、vbo_writtenをtrueに設定し、
 *   VBO Allocationの一時的な解放責務を保持する。
 *
 * Draw Range Validation:
 * - VBO Allocation Offsetがsizeof(point_normal_vertex_t)の
 *   倍数であることを直接検査する。
 * - 不整合を検出した場合はDATA_CORRUPTEDとする。
 * - 条件成立後、Allocation Offsetからfirst_vertex_countを算出する。
 * - Draw Rangeのvertex_countにはLoaderが返した
 *   Vertex Countを設定する。
 * - Allocation OffsetのStride整合性は本Pipelineが判定し、
 *   Geometry Registryのcanonical validationへ委譲しない。
 *
 * Temporary Vertex Release:
 * - VBO転送とDraw Range構築後、
 *   Loaderから取得した一時頂点配列を解放する。
 * - 解放にはgeneral_allocator_free()を使用し、
 *   GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRYを指定する。
 * - Registryへ登録するCPU Geometryは独立した
 *   頂点storageを所有しているため、この解放の影響を受けない。
 * - RegistryへはLoaderの一時頂点配列を渡さない。
 *
 * Commit:
 * - CPU GeometryとVBO Allocation Descriptorを
 *   untextured_mesh_geometry_registry_register()へ渡す。
 * - 登録成功によって両Resourceの解放責務はRegistryへ移転する。
 * - RegistryがReference Countを1に初期化し、
 *   callerに最初の利用権を付与する。
 * - 登録成功後にGeometry IDを出力する。
 *
 * Failure / Cleanup:
 * - Commit前の回復可能な失敗では、
 *   VBO Allocationの確保が成功していればrollbackする。
 * - VBO Allocationのrollbackが成功した場合、
 *   一時的に所有しているCPU Geometryをdestroyする。
 * - Loaderの一時頂点配列が残っている場合は、
 *   General Allocatorを使用して解放する。
 * - VBO Allocationのrollbackに失敗した場合は
 *   DATA_CORRUPTEDを返し、以降のcleanupを中断する。
 * - DATA_CORRUPTEDの場合は通常のcleanupを行わない。
 * - この場合、確保済みResourceの解放完了は保証しない。
 * - Registryのregister()がDATA_CORRUPTEDを返した場合も、
 *   Ownership Transferの状態を安全に判断できないため、
 *   通常のcleanupを行わない。
 * - 回復可能な失敗ではout_geometry_id_を変更しない。
 *
 * Postconditions:
 * - 登録成功時にはGeometry Registry側で
 *   Registered Stateの成立を保証する。
 * - Pipeline側ではRegistryのcanonical validationを
 *   重複して実行しない。
 * - 成功時、一時的なCPU Geometryの所有ポインタは
 *   RegistryへのOwnership TransferによってNULLとなる。
 * - 成功時、Loaderの一時頂点配列は解放済みとなる。
 */
resource_pipeline_result_t untextured_mesh_geometry_pipeline_import_from_file(vbo_manager_t* vbo_manager_, untextured_mesh_geometry_registry_t* geometry_registry_, const char* resource_name_, const char* resource_fullpath_, uint16_t* out_geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    loader_result_t ret_loader = LOADER_INVALID_ARGUMENT;
    geometry_result_t ret_geometry = GEOMETRY_INVALID_ARGUMENT;
    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    buffer_manager_result_t ret_buffer_manager = BUFFER_MANAGER_INVALID_ARGUMENT;

    point_normal_vertex_t* vertices = NULL;
    size_t vertex_count = 0;
    size_t write_size = 0;
    uint16_t tmp_geometry_id = 0;
    vbo_range_t tmp_buffer_range = { 0 };
    bool vbo_written = false;
    bool exists = false;

    untextured_mesh_geometry_t* geometry = NULL;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_import_from_file", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_import_from_file", "geometry_registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_geometry_id_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_import_from_file", "out_geometry_id_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_import_from_file", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_fullpath_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_import_from_file", "resource_fullpath_")

    ret_resource_registry = untextured_mesh_geometry_registry_exists(geometry_registry_, resource_name_, &exists);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - untextured_mesh_geometry_registry_exists failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }
    if(exists) {
        ret_resource_registry = untextured_mesh_geometry_registry_acquire(geometry_registry_, resource_name_, &tmp_geometry_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
            ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - untextured_mesh_geometry_registry_acquire failed.", resource_pipeline_result_to_str(ret));
            goto cleanup;
        }
    } else {
        ret_loader = stl_ascii_loader_load(resource_fullpath_, &vertex_count, &vertices);
        if(LOADER_SUCCESS != ret_loader) {
            ret = resource_pipeline_result_convert_loader(ret_loader);
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - Failed to import untextured mesh geometry. reason=stl_load_failed, geometry_name='%s'", resource_pipeline_result_to_str(ret), resource_name_);
            goto cleanup;
        }

        ret_geometry = untextured_mesh_geometry_create_from_vertices(vertex_count, vertices, &geometry);
        if(GEOMETRY_SUCCESS != ret_geometry) {
            ret = resource_pipeline_result_convert_geometry(ret_geometry);
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - Failed to import untextured mesh geometry. reason=geometry_create_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count);
            goto cleanup;
        }

        if(!size_mul_checked(vertex_count, sizeof(point_normal_vertex_t), &write_size)) {
            ret = RESOURCE_PIPELINE_OVERFLOW;
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - vbo write size overflow failed.", resource_pipeline_result_to_str(ret));
            goto cleanup;
        }
        ret_buffer_manager = vbo_manager_write(vbo_manager_, write_size, vertices, &tmp_buffer_range.allocation_info);
        if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
            ret = resource_pipeline_result_convert_buffer_manager(ret_buffer_manager);
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - Failed to import untextured mesh geometry. reason=vbo_write_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count);
            goto cleanup;
        }
        vbo_written = true;
        if(0 != (tmp_buffer_range.allocation_info.offset % sizeof(point_normal_vertex_t))) {
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            goto cleanup;
        }
        tmp_buffer_range.draw_range.first_vertex_count = tmp_buffer_range.allocation_info.offset / sizeof(point_normal_vertex_t);
        tmp_buffer_range.draw_range.vertex_count = vertex_count;
        general_allocator_free((void**)&vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);

        ret_resource_registry = untextured_mesh_geometry_registry_register(geometry_registry_, resource_name_, &geometry, &tmp_buffer_range, &tmp_geometry_id);
        if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
            ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - Failed to import untextured mesh geometry. reason=geometry_register_failed, geometry_name='%s', vertex_count=%zu", resource_pipeline_result_to_str(ret), resource_name_, vertex_count);
            goto cleanup;
        }
    }

    *out_geometry_id_ = tmp_geometry_id;

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    if(RESOURCE_PIPELINE_DATA_CORRUPTED == ret) {
        return ret;
    }
    if(RESOURCE_PIPELINE_SUCCESS != ret && vbo_written) {
        ret_buffer_manager = vbo_manager_free(vbo_manager_, &tmp_buffer_range.allocation_info);
        if(BUFFER_MANAGER_SUCCESS != ret_buffer_manager) {
            ret = RESOURCE_PIPELINE_DATA_CORRUPTED;
            ERROR_MESSAGE("untextured_mesh_geometry_pipeline_import_from_file(%s) - VBO rollback failed.", resource_pipeline_result_to_str(ret));
            return ret;
        }
    }
    untextured_mesh_geometry_destroy(&geometry);
    if(NULL != vertices) {
        general_allocator_free((void**)&vertices, GENERAL_ALLOCATOR_MEMORY_TAG_GEOMETRY);
    }

    return ret;
}

/*
 * API-specific Validation Policy
 *
 * Preconditions:
 * - vbo_manager_とgeometry_registry_のNULLを
 *   全Build Modeで検査する。
 * - Geometry IDの範囲、対象EntryのRegistered State、
 *   およびRegistryのcanonical validityは、
 *   untextured_mesh_geometry_registry_release()へ委譲する。
 * - VBO Managerの内部整合性とAllocationの解放条件は、
 *   Registryを経由してVBO Manager自身のAPIに委譲する。
 * - Pipeline側ではこれらのValidationを重複して実行しない。
 *
 * Commit:
 * - Geometry Registryのrelease()を呼び出し、
 *   指定Geometryの利用権を1つ返却する。
 * - Reference Countが2以上の場合、RegistryはCountを
 *   1減少させ、Resourceの登録状態を維持する。
 * - Reference Countが1の場合、Registryは対応する
 *   VBO AllocationとCPU Geometryを解放し、
 *   対象EntryをEmpty Stateへ戻す。
 * - 本PipelineはReference CountやEntryを直接変更しない。
 *
 * Result:
 * - Registryのrelease()が返したResultを
 *   Resource PipelineのResult Codeへ変換する。
 * - Registryのrelease()が成功した場合は
 *   RESOURCE_PIPELINE_SUCCESSを返す。
 * - 下位Operationの失敗は変換後のResult Codeとして返す。
 *
 * Postconditions:
 * - Registryのstable state成立はRegistry側の
 *   release() contractに委譲する。
 * - Pipeline側ではCommit後のcanonical validationを
 *   重複して実行しない。
 *
 * Failure:
 * - Pipeline自身によるResourceの確保やOwnership Transferは
 *   発生しないため、独立したrollback処理を持たない。
 * - Commit前の引数不正ではResourceを変更しない。
 * - 下位RegistryがDATA_CORRUPTEDを返した場合、
 *   Resourceの解放完了および内部状態の復元を保証しない。
 */
resource_pipeline_result_t untextured_mesh_geometry_pipeline_release(vbo_manager_t* vbo_manager_, untextured_mesh_geometry_registry_t* geometry_registry_, uint16_t geometry_id_) {
    resource_pipeline_result_t ret = RESOURCE_PIPELINE_INVALID_ARGUMENT;

    resource_registry_result_t ret_resource_registry = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(vbo_manager_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_release", "vbo_manager_")
    IF_ARG_NULL_GOTO_CLEANUP(geometry_registry_, ret, RESOURCE_PIPELINE_INVALID_ARGUMENT, resource_pipeline_result_to_str(RESOURCE_PIPELINE_INVALID_ARGUMENT), "untextured_mesh_geometry_pipeline_release", "geometry_registry_")

    ret_resource_registry = untextured_mesh_geometry_registry_release(geometry_registry_, vbo_manager_, geometry_id_);
    if(RESOURCE_REGISTRY_SUCCESS != ret_resource_registry) {
        ret = resource_pipeline_result_convert_resource_registry(ret_resource_registry);
        ERROR_MESSAGE("untextured_mesh_geometry_pipeline_release(%s) - untextured_mesh_geometry_registry_release failed.", resource_pipeline_result_to_str(ret));
        goto cleanup;
    }

    ret = RESOURCE_PIPELINE_SUCCESS;

cleanup:
    return ret;
}
