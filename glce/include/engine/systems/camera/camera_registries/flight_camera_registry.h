// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/**
 * @file flight_camera_registry.h
 * @author chocolate-pie24
 * @brief Flight Cameraの登録、検索、取得、および登録解除を提供する。
 *
 * @details
 * Flight CameraをResource NameおよびFlight Camera IDによって
 * 管理するRegistry。
 * Flight Cameraの生成、カメラ操作、および入力イベントの処理は担当しない。
 *
 * @section flight_camera_registry_boundary_contract Module Boundary Contract
 *
 * @par Ownership
 * - Registryは登録されたFlight Cameraの所有権を持つ。
 * - Registryは登録時にResource Nameを複製し、その文字列を所有する。
 * - 登録成功後、callerは移転したFlight Cameraを破棄してはならない。
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
 * - Flight Camera IDはRegistry内でのみ意味を持ち、
 *   異なるRegistry間で共有しない。
 * - 登録解除したEntryのIDは、後の登録で再利用される場合がある。
 * - Getterが返すFlight Cameraへのポインタはmutable borrowであり、
 *   ownershipはRegistryに残る。
 * - callerはborrowを通じてFlight Cameraの操作を行ってよいが、
 *   そのResourceを破棄してはならない。
 * - 対象Entryの登録解除、またはRegistryのlifetime終了によって
 *   borrowは無効になる。
 *
 * @par AI支援:
 * - 本DocumentationはChatGPTによる草案である。
 * - 実装コードはプロジェクト作成者が作成した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_FLIGHT_CAMERA_REGISTRY_H
#define GLCE_ENGINE_SYSTEMS_CAMERA_CAMERA_REGISTRIES_FLIGHT_CAMERA_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/camera/camera_registries/core/camera_registry_types.h"


typedef struct flight_camera_registry flight_camera_registry_t;

typedef struct subsystem_allocator subsystem_allocator_t;
typedef struct flight_camera flight_camera_t;

/**
 * @brief Flight Camera Registryを生成する。
 *
 * @par Public API Contract
 * - 最大登録数は1以上UINT16_MAX以下とし、出力先はNULLで初期化しておく。
 * - 成功時には、指定容量の未登録Entryを持つ利用可能なRegistryを生成する。
 * - 回復可能な構築失敗では、今回の構築によるAllocatorのallocation stateを
 *   rollbackし、出力先をNULLのまま維持する。
 * - DATA_CORRUPTEDの場合は通常のrollbackを行わない。
 *
 * @param[in] max_flight_camera_count_ 最大登録可能Flight Camera数。
 * @param[in,out] allocator_ Registryのstorageを提供するSubsystem Allocator。
 * @param[out] out_registry_ 生成したRegistryを受け取る出力先。
 *
 * @retval CAMERA_REGISTRY_SUCCESS 生成成功。
 * @retval CAMERA_REGISTRY_INVALID_ARGUMENT 引数または最大登録数が不正。
 * @retval CAMERA_REGISTRY_BAD_OPERATION 出力先がNULLでない、または下位Allocatorが操作を拒否した。
 * @retval CAMERA_REGISTRY_OVERFLOW 配列サイズ計算などでoverflowが発生。
 * @retval CAMERA_REGISTRY_NO_MEMORY メモリ確保に失敗。
 * @retval CAMERA_REGISTRY_LIMIT_EXCEEDED 下位Allocatorの利用可能範囲を超えた。
 * @retval CAMERA_REGISTRY_DATA_CORRUPTED 内部状態の検証またはrollbackに失敗。
 * @retval CAMERA_REGISTRY_UNDEFINED_ERROR 下位Allocatorから未分類の失敗を受けた。
 *
 * @pre allocator_は利用可能なSubsystem Allocatorを指していること。
 * @post 成功時、*out_registry_は通常操作に使用できるRegistryを指す。
 */
camera_registry_result_t flight_camera_registry_create(size_t max_flight_camera_count_, subsystem_allocator_t* allocator_, flight_camera_registry_t** out_registry_);

/**
 * @brief Registryの利用可能状態を終了する。
 *
 * @par Public API Contract
 * - 登録済みのResource NameおよびFlight Cameraをすべて破棄する。
 * - Registry本体とEntry配列のbacking storageは解放しない。
 * - 正常終了後はRegistryを再利用できず、再度のdeinitializeも許可しない。
 *
 * @param[in,out] registry_ 終了対象のRegistry。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @post 正常終了後、Registryのlogical lifetimeは終了する。
 */
void flight_camera_registry_deinitialize(flight_camera_registry_t* registry_);

/**
 * @brief 指定したResource NameのFlight Cameraが登録されているか判定する。
 *
 * @par Public API Contract
 * - name_は空文字列ではないNUL終端文字列とする。
 * - Registryの登録状態は変更しない。
 * - falseは未登録のほか、引数不正または内部状態の検証失敗も表し得る。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 *
 * @retval true 指定名のFlight Cameraが登録されている。
 * @retval false 指定名が未登録、または検索を実行できなかった。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 */
bool flight_camera_registry_exists(const flight_camera_registry_t* registry_, const char* name_);

/**
 * @brief Flight Camera IDに対応するFlight Cameraを取得する。
 *
 * @par Public API Contract
 * - IDはRegistryの登録可能範囲内であり、対象Entryが登録済みである必要がある。
 * - 成功時にはRegistryが所有するFlight Cameraへのmutable borrowを返す。
 * - callerは返されたポインタを通じてFlight Cameraを操作してよいが、
 *   Resource自体を破棄してはならない。
 * - 対象Entryの登録解除またはRegistryのlifetime終了によって
 *   borrowは無効になる。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] flight_camera_id_ Flight Cameraを取得するID。
 *
 * @return 成功時はFlight Cameraへのポインタ。失敗時はNULL。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 */
flight_camera_t* flight_camera_registry_flight_camera_get(const flight_camera_registry_t* registry_, uint16_t flight_camera_id_);

/**
 * @brief Resource Nameに対応するFlight Camera IDを取得する。
 *
 * @par Public API Contract
 * - name_は空文字列ではないNUL終端文字列とする。
 * - 指定名が登録済みなら対応するIDを出力する。
 * - 指定名が未登録の場合はBAD_OPERATIONを返す。
 * - 失敗時に出力先の値は変更しない。Registryの登録状態も変更しない。
 *
 * @param[in] registry_ 検索対象のRegistry。
 * @param[in] name_ 検索するResource Name。
 * @param[out] out_flight_camera_id_ 取得したFlight Camera IDの出力先。
 *
 * @retval CAMERA_REGISTRY_SUCCESS 取得成功。
 * @retval CAMERA_REGISTRY_INVALID_ARGUMENT 引数が不正。
 * @retval CAMERA_REGISTRY_BAD_OPERATION 指定名のFlight Cameraが未登録。
 * @retval CAMERA_REGISTRY_DATA_CORRUPTED Registryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre name_は読み取り可能なNUL終端文字列を指していること。
 * @post 成功時、出力IDは指定名の登録済みFlight Cameraを識別する。
 */
camera_registry_result_t flight_camera_registry_id_get(const flight_camera_registry_t* registry_, const char* name_, uint16_t* out_flight_camera_id_);

/**
 * @brief Flight Cameraの所有権を受け取り、Registryへ登録する。
 *
 * @par Public API Contract
 * - flight_camera_name_は空文字列ではないNUL終端文字列とし、
 *   同名の重複登録は許可しない。
 * - 入力Flight Cameraは、Flight Camera Moduleが定義する
 *   canonical validityを満たし、callerが移転可能な所有権を保持していること。
 * - 成功時、Resource Nameを複製し、Flight Cameraの所有権をRegistryへ移す。
 *   *flight_camera_をNULLにし、登録IDを出力する。
 * - 登録可能なEntryがない場合はLIMIT_EXCEEDEDを返す。
 * - 解除済みEntryのFlight Camera IDが再利用される場合がある。
 *
 * @par Failure Contract
 * - 回復可能な失敗ではRegistryの登録状態、入力Flight Cameraの所有権と
 *   *flight_camera_、および出力IDを変更しない。
 * - DATA_CORRUPTEDの場合は、Commit後の状態復元や
 *   Flight Camera所有権の返却を保証しない。
 *
 * @param[in,out] registry_ 登録先のRegistry。
 * @param[in] flight_camera_name_ 登録するResource Name。
 * @param[in,out] flight_camera_ Flight Cameraの所有権を移転するポインタ変数へのポインタ。
 * @param[out] out_flight_camera_id_ 登録したFlight Camera IDの出力先。
 *
 * @retval CAMERA_REGISTRY_SUCCESS 登録成功。
 * @retval CAMERA_REGISTRY_INVALID_ARGUMENT NULL引数または不正なResource Name。
 * @retval CAMERA_REGISTRY_BAD_OPERATION 入力Flight CameraがNULL、またはResource Nameが登録済み。
 * @retval CAMERA_REGISTRY_LIMIT_EXCEEDED 登録可能なEntryがない、または下位処理の上限超過。
 * @retval CAMERA_REGISTRY_NO_MEMORY Resource Nameの生成に必要なメモリを確保できない。
 * @retval CAMERA_REGISTRY_OVERFLOW Resource Name生成などでoverflowが発生。
 * @retval CAMERA_REGISTRY_RUNTIME_ERROR 下位の文字列生成処理でruntime errorが発生。
 * @retval CAMERA_REGISTRY_DATA_CORRUPTED 内部状態または登録後状態の検証に失敗。
 * @retval CAMERA_REGISTRY_UNDEFINED_ERROR 下位処理から未分類の失敗を受けた。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @pre flight_camera_name_は読み取り可能なNUL終端文字列を指していること。
 * @pre *flight_camera_は有効なFlight Cameraの移転可能な所有権を表し、
 *      同じResourceを複数Ownerで管理していないこと。
 * @post 成功時、RegistryがFlight Cameraを所有し、出力IDから取得できる。
 */
camera_registry_result_t flight_camera_registry_register(flight_camera_registry_t* registry_, const char* flight_camera_name_, flight_camera_t** flight_camera_, uint16_t* out_flight_camera_id_);

/**
 * @brief Flight Camera IDに対応するFlight Cameraを登録解除する。
 *
 * @par Public API Contract
 * - flight_camera_id_はRegistryの登録可能範囲内である必要がある。
 * - 未登録IDはBAD_OPERATIONとして拒否する。
 * - 成功時、対象EntryのResource NameとFlight Cameraを破棄し、
 *   登録状態を解除する。
 * - 登録解除したIDおよび対象Entryから取得済みのborrowは無効になる。
 * - 解除したIDは、後の登録で再利用される場合がある。
 *
 * @param[in,out] registry_ 登録解除対象のRegistry。
 * @param[in] flight_camera_id_ 登録解除するFlight Camera ID。
 *
 * @retval CAMERA_REGISTRY_SUCCESS 登録解除成功。
 * @retval CAMERA_REGISTRY_INVALID_ARGUMENT NULL引数またはIDの範囲が不正。
 * @retval CAMERA_REGISTRY_BAD_OPERATION 指定IDに解除可能なEntryがない。
 * @retval CAMERA_REGISTRY_DATA_CORRUPTED 対象Entryの整合性検証に失敗。
 *
 * @pre registry_は利用可能状態にあるRegistryを指していること。
 * @post 成功時、指定IDに対応するEntryは未登録状態となる。
 */
camera_registry_result_t flight_camera_registry_unregister(flight_camera_registry_t* registry_, uint16_t flight_camera_id_);

/**
 * @brief Flight Camera Registryのcanonical validityを判定する。
 *
 * @par Validation Scope
 * - Registryの最大登録数が1以上UINT16_MAX以下であり、Entry配列が非NULLである。
 * - Entry配列の論理サイズ計算でoverflowが発生しない。
 * - 各Entryは、未登録状態（Resource Name / Flight CameraがともにNULL）、
 *   または登録状態（両方とも非NULL）として整合している。
 * - 登録状態の各Owned PointerはGeneral Allocator上の現在のallocationである。
 * - 登録状態のResource NameはvalidなChoco Stringであり、空文字列ではない。
 * - 登録状態のFlight Cameraはcanonical validityを満たす。
 *
 * Resource NameのEntry間一意性、Registry本体・Entry配列の
 * allocation-level validity、およびSubsystem Allocatorの内部状態は検証しない。
 *
 * @param[in] registry_ 検証対象のRegistry。
 *
 * @retval true 対象Registryのcanonical invariantを満たす。
 * @retval false NULLを含め、いずれかの条件を満たさない。
 *
 * @pre NULL以外のregistry_を指定する場合、Registry本体およびEntry配列のstorageは
 *      検証処理に必要な範囲で読み取り可能であること。
 */
bool flight_camera_registry_is_valid(const flight_camera_registry_t* registry_);

#ifdef __cplusplus
}
#endif
#endif
