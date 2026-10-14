// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup renderer
 *
 * @file texture_registry.h
 * @author chocolate-pie24
 * @brief Texture Resourceの登録、検索、取得、および登録解除を提供する。
 *
 * @details
 * Texture CPU ResourceとTexture GPU Resourceを対として登録し、
 * Resource NameおよびTexture IDによって管理するRegistry。
 * Texture Resourceの生成、画像の読み込み、GPUへの転送、描画は担当しない。
 *
 * @section texture_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは、登録されたTexture CPU ResourceとTexture GPU Resourceの所有権を持つ。
 * - Registryは登録時にResource Nameを複製し、その文字列を所有する。
 * - 登録成功後、callerは移転したResourceを破棄してはならない。
 * - 登録解除またはRegistryのdeinitialize時に、Registryが所有Resourceを破棄する。
 *
 * @par Storage / Lifetime
 * - Registry本体およびEntry配列のstorageは、callerから提供された
 *   Subsystem Allocatorのbacking memoryから確保する。
 * - Registryはこれらのbacking storageを個別に解放しない。
 * - callerはRegistryの利用期間中、当該storageとAllocatorのlifetimeを保証する。
 * - deinitialize後のRegistryは再利用できない。
 *
 * @par Resource Identity / Borrow
 * - Texture IDはRegistry内でのみ意味を持ち、異なるRegistry間で共有しない。
 * - 登録解除したEntryのIDは、後の登録で再利用される場合がある。
 * - Getterが返すResource NameとCPU / GPU Resourceへのポインタはconst borrowであり、
 *   ownershipはRegistryに残る。callerはborrowを変更・解放してはならない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によってborrowは無効になる。
 *
 * @par AI支援
 * - 本セクションはChatGPTを用いて草案を作成し、
 *   プロジェクト作成者が実装との整合性を確認・修正した。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_TEXTURE_TEXTURE_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCE_REGISTRIES_TEXTURE_TEXTURE_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"

typedef struct texture_registry texture_registry_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct texture_gpu_resource texture_gpu_resource_t;
typedef struct texture_cpu_resource texture_cpu_resource_t;
typedef struct renderer_backend_context renderer_backend_context_t;

/**
 * @brief Texture Registryを生成する。
 *
 * @par Public API Contract
 * - 最大登録数は1以上UINT16_MAX以下とし、出力先はNULLで初期化しておく。
 * - 成功時には、指定容量の未登録Entryを持つ利用可能なRegistryを生成する。
 * - 回復可能な構築失敗では、今回の構築によるAllocatorのallocation stateを
 *   rollbackし、出力先をNULLのまま維持する。
 * - DATA_CORRUPTEDの場合は通常のrollbackを行わない。
 *
 * @param[in] max_texture_count_ 最大登録可能Texture数。
 * @param[in,out] allocator_ Registryのstorageを提供するSubsystem Allocator。
 * @param[out] out_registry_ 生成したRegistryを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 生成成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数または最大登録数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 出力先がNULLでない、または下位Allocatorが操作を拒否した。
 * @retval RESOURCE_REGISTRY_OVERFLOW 配列サイズ計算などでoverflowが発生。
 * @retval RESOURCE_REGISTRY_NO_MEMORY メモリ確保に失敗。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 下位Allocatorの利用可能範囲を超えた。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 内部状態の検証またはrollbackに失敗。
 * @retval RESOURCE_REGISTRY_UNDEFINED_ERROR 下位Allocatorから未分類の失敗を受けた。
 *
 * @pre allocator_は利用可能なSubsystem Allocatorを指していること。
 * @post 成功時、*out_registry_は通常操作に使用できるRegistryを指す。
 */
resource_registry_result_t texture_registry_create(size_t max_texture_count_, subsystem_allocator_t* allocator_, texture_registry_t** out_registry_);

/**
 * @brief Registryの利用可能状態を終了する。
 *
 * @par Public API Contract
 * - 登録済みのResource NameおよびCPU / GPU Resourceをすべて破棄する。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 * - 正常終了後はRegistryを再利用できず、再度のdeinitializeも許可しない。
 *
 * @param[in,out] registry_ 終了対象のRegistry。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @post 正常終了後、Registryのlogical lifetimeは終了する。
 */
void texture_registry_deinitialize(texture_registry_t* registry_);

/**
 * @brief 指定したResource NameのTextureが登録されているか判定する。
 *
 * @par Public API Contract
 * - name_は空文字列ではないNUL終端文字列とする。
 * - Registryの登録状態は変更しない。
 * - falseは未登録のほか、引数不正または内部状態の検証失敗も表し得る。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 *
 * @retval true 指定名のTextureが登録されている。
 * @retval false 指定名が未登録、または検索を実行できなかった。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 */
bool texture_registry_exists(const texture_registry_t* registry_, const char* name_);

/**
 * @brief Texture IDに対応するResource Nameを取得する。
 *
 * @details
 * 成功時はRegistryが所有するResource Nameへのconst borrowを返す。
 * IDが範囲外、または対象Entryが未登録の場合はNULLを返す。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] texture_id_ Resource Nameを取得するTexture ID。
 *
 * @return 成功時はResource Nameへのconstポインタ。失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const char* texture_registry_name_get(const texture_registry_t* registry_, uint16_t texture_id_);

/**
 * @brief Texture IDに対応するGPU Resourceを取得する。
 *
 * @details
 * 成功時はRegistryが所有するGPU Resourceへのconst borrowを返す。
 * IDが範囲外、または対象Entryが未登録の場合はNULLを返す。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] texture_id_ GPU Resourceを取得するTexture ID。
 *
 * @return 成功時はGPU Resourceへのconstポインタ。失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const texture_gpu_resource_t* texture_registry_gpu_resource_get(const texture_registry_t* registry_, uint16_t texture_id_);

/**
 * @brief Texture IDに対応するCPU Resourceを取得する。
 *
 * @details
 * 成功時はRegistryが所有するCPU Resourceへのconst borrowを返す。
 * IDが範囲外、または対象Entryが未登録の場合はNULLを返す。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] texture_id_ CPU Resourceを取得するTexture ID。
 *
 * @return 成功時はCPU Resourceへのconstポインタ。失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
const texture_cpu_resource_t* texture_registry_cpu_resource_get(const texture_registry_t* registry_, uint16_t texture_id_);

/**
 * @brief Resource Nameに対応するTexture IDを取得する。
 *
 * @par Public API Contract
 * - name_は空文字列ではないNUL終端文字列とする。
 * - 指定名が登録済みの場合、対応するTexture IDを出力する。
 * - 指定名が未登録の場合はBAD_OPERATIONを返す。
 * - 失敗時は出力先の値を変更しない。
 * - Registryの登録状態は変更しない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 * @param[out] out_texture_id_ Texture IDを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 取得成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 指定名のTextureが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 * @pre out_texture_id_が非NULLの場合、書き込み可能なuint16_tオブジェクトを指していること。
 *
 * @post 成功時、出力IDは指定名の登録済みTextureを識別する。
 */
resource_registry_result_t texture_registry_id_get(const texture_registry_t* registry_, const char* name_, uint16_t* out_texture_id_);

/**
 * @brief Texture CPU / GPU ResourceをRegistryへ登録する。
 *
 * @par Public API Contract
 * - 指定したResource Nameが未登録であり、空きEntryが存在する場合に登録する。
 * - Resource NameはRegistry内部へ複製する。
 * - CPU / GPU Resourceは有効なStable Stateにあることをcallerが保証する。
 * - 成功時、CPU / GPU Resourceの所有権はRegistryへ移転し、
 *   *cpu_resource_および*gpu_resource_はNULLとなる。
 * - 成功時、登録したTextureのIDをout_texture_id_へ出力する。
 * - 回復可能な失敗では、Registryの登録状態およびcallerのResource所有権を変更せず、
 *   出力先のTexture IDも変更しない。
 * - DATA_CORRUPTEDはfail-stop対象であり、Commit後に検出された場合、
 *   Registryの状態復元およびcallerの所有権状態の復元を保証しない。
 *
 * @param[in,out] registry_ 登録先のRegistry。
 * @param[in] resource_name_ 登録するResource Name。
 * @param[in,out] gpu_resource_ 登録するGPU Resourceの所有ポインタ。
 * @param[in,out] cpu_resource_ 登録するCPU Resourceの所有ポインタ。
 * @param[out] out_texture_id_ 登録したTextureのIDを受け取る出力先。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT 引数またはResource Nameが不正。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION ResourceがNULL、または指定名が登録済み。
 * @retval RESOURCE_REGISTRY_LIMIT_EXCEEDED 空きEntryが存在しない。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre resource_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre CPU / GPU Resourceは、それぞれのModuleが定義する
 *      Canonical Invariantを満たし、callerが正当な所有権を保持していること。
 * @pre out_texture_id_が非NULLの場合、書き込み可能なuint16_tオブジェクトを指していること。
 *
 * @post 成功時、Registryが登録されたResource NameとCPU / GPU Resourceを所有する。
 * @post 成功時、出力IDは新たに登録されたTextureを識別する。
 */
resource_registry_result_t texture_registry_register(texture_registry_t* registry_, const char* resource_name_, texture_gpu_resource_t** gpu_resource_, texture_cpu_resource_t** cpu_resource_, uint16_t* out_texture_id_);

/**
 * @brief 指定したTexture IDの登録を解除する。
 *
 * @par Public API Contract
 * - 指定IDに対応する登録済みEntryを解除対象とする。
 * - 登録解除時、Registryが所有するResource NameとCPU / GPU Resourceを破棄する。
 * - 対象Entryが未登録の場合、BAD_OPERATIONを返す。
 * - 成功時、対象Entryは未登録状態となり、そのTexture IDは再利用可能となる。
 * - 登録解除されたTextureに対して取得済みのBorrowは無効になる。
 * - 失敗時は、登録解除処理を実行せず、対象Entryの状態を変更しない。
 *
 * @param[in,out] registry_ 登録解除対象のRegistry。
 * @param[in] texture_id_ 登録解除するTexture ID。
 *
 * @retval RESOURCE_REGISTRY_SUCCESS 登録解除成功。
 * @retval RESOURCE_REGISTRY_INVALID_ARGUMENT RegistryがNULL、またはTexture IDが範囲外。
 * @retval RESOURCE_REGISTRY_BAD_OPERATION 対象Entryが未登録。
 * @retval RESOURCE_REGISTRY_DATA_CORRUPTED 対象Entryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 *
 * @post 成功時、対象Entryは未登録状態となる。
 * @post 成功時、対象Entryに属していたResource NameとCPU / GPU Resourceは破棄済みとなる。
 */
resource_registry_result_t texture_registry_unregister(texture_registry_t* registry_, uint16_t texture_id_);

/**
 * @brief Texture RegistryのCanonical Validityを判定する。
 *
 * @details
 * Registryの構造と、登録済みResourceを含むStable Stateが
 * Canonical Invariantを満たしているか判定する。
 *
 * @par Validation Scope
 * - 最大登録数が1以上UINT16_MAX以下であること。
 * - Entry配列のポインタが非NULLであること。
 * - 各Entryが次のいずれかの状態にあること。
 *   - Empty State: Resource NameとCPU / GPU ResourceがすべてNULL。
 *   - Registered State: 3フィールドすべてが非NULL。
 * - Registered StateのOwned Pointerが、General Allocatorによる
 *   有効なAllocationを参照していること。
 * - Resource Nameが有効な文字列であり、空文字列ではないこと。
 * - CPU / GPU ResourceがそれぞれのCanonical Invariantを満たすこと。
 *
 * @par Validation Limitations
 * - Resource NameのEntry間一意性は検証しない。
 * - Registry本体およびEntry配列の実際のAllocation Sizeは検証しない。
 *
 * @param[in] registry_ 検証対象のRegistry。
 *
 * @retval true RegistryがCanonical Invariantを満たしている。
 * @retval false RegistryがNULL、またはCanonical Invariantを満たしていない。
 *
 * @pre 非NULLのregistry_は読み取り可能な有効lifetimeのObjectを指すこと。
 * @pre Entry配列は、max_texture_count個のEntryを読み取り可能な
 *      有効lifetimeのStorageを指すこと。
 */
bool texture_registry_is_valid(const texture_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
