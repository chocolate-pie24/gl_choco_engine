/**
 * @file vbo_manager.h
 * @brief GPU側VBOとその内部rangeを管理するVBO Managerの公開API
 *
 * @details
 * VBO Managerは、一つのRenderer Backend VBOと一つのRange Allocatorを所有し、
 * GPU側VBOの生成、データ転送、およびVBO内rangeの確保と解放を管理する。
 *
 * GPU buffer objectに対する操作はRenderer Backendへ委譲し、
 * VBO内の論理的なoffset、実確保サイズ、およびallocation identityは
 * Range Allocatorを使用して管理する。
 *
 * @par VBOの構成
 * vbo_manager_create()の実行時に、VBO全体のサイズ、同時に生存できる
 * allocation数の上限、固定base alignment、およびbuffer usageを設定する。
 *
 * create成功後にこれらの設定を変更することはできない。
 * VBO Managerごとに一つのVBOとRange Allocatorを使用し、
 * 異なるalignment要件を同じVBO Manager内へ混在させない。
 *
 * @par Writeとallocation
 * vbo_manager_write()は、Range Allocatorから要求サイズを収容できるrangeを確保し、
 * そのoffsetへデータを転送する。
 *
 * Range Allocatorが確保するrangeのサイズは、要求サイズをbase alignment単位に
 * 切り上げた実確保サイズである。GPUへ転送するデータサイズは呼び出し側が
 * 指定した要求サイズであり、alignment padding部分への転送は行わない。
 *
 * write成功時には、確保したrangeを表すvertex_allocation_tを呼び出し側へ返す。
 * このallocation handleは、対応するVBO Managerが生存し、
 * allocationがfreeされるまで有効である。
 *
 * @par Free
 * vbo_manager_free()は、allocation handleが表すrangeをRange Allocatorへ返却する。
 * GPU側VBOに書き込まれたデータの消去またはゼロクリアは行わない。
 *
 * free成功後もallocation handle自体の内容は変更されないが、
 * live allocationを表さなくなるため、再度freeへ渡してはならない。
 *
 * @par Rollback
 * vbo_manager_create()は、VBO Manager、Range Allocator、およびRenderer Backend VBOの
 * 生成がすべて完了した場合にのみ、生成したVBO Managerを呼び出し側へ公開する。
 * 途中で失敗した場合は、生成済みのリソースを逆順に解放する。
 *
 * vbo_manager_write()は、range確保後にbind、データ転送、またはunbindへ失敗した場合、
 * 確保済みrangeをfreeし、VBOのbinding状態を復旧してから失敗を通知する。
 * allocation handleはwrite全体が成功した場合にのみ更新される。
 *
 * rollback中に、正常な内部状態では成功するはずのrange解放またはunbindに
 * 失敗した場合は、内部状態を保証できないためBUFFER_MANAGER_DATA_CORRUPTEDを返す。
 *
 * write失敗時にGPU側へデータが一部転送されていても、allocation handleは
 * 公開されず、対応rangeは再利用対象となるため、転送前のデータへの復元は行わない。
 *
 * @par Ownershipと生存期間
 * VBO Managerは、内部のRange AllocatorおよびRenderer Backend VBOを所有する。
 * これらのリソースはvbo_manager_destroy()によってVBO Managerとともに破棄される。
 *
 * VBO Managerを破棄すると、そのVBO Managerから取得したすべての
 * vertex_allocation_tは無効となる。
 *
 * @date 2026-07-31
 *
 * @copyright Copyright (c) 2026 chocolate-pie24
 *
 * @par License
 * MIT License. See LICENSE file in the project root for full license text.
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_BUFFER_MANAGERS_VBO_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "engine/systems/renderer/core/renderer_types.h"
#include "engine/systems/renderer/resources/allocators/core/range_allocator_types.h"

#include "engine/systems/renderer/resources/buffer_managers/core/buffer_manager_types.h"

/**
 * @brief VBO Managerの生成設定
 *
 * @details
 * vbo_manager_create()によって生成するVBO Managerの容量、
 * allocation数上限、固定base alignment、およびbuffer usageを指定する。
 *
 * create成功時に本構造体の内容はVBO Manager内部へコピーされる。
 * VBO Managerは本構造体へのpointerを保持しないため、呼び出し後も
 * 呼び出し側が本構造体の所有権を保持する。
 *
 * 本構造体の基本的な有効性は、
 * vbo_manager_config_is_valid()によって検証できる。
 *
 * @par VBO size
 * vbo_sizeは、生成するGPU側VBOとRange Allocatorが管理する
 * 論理的なmemory poolの総サイズをbyte単位で表す。
 *
 * 0は指定できない。
 * base_alignの倍数であることは要求しないため、VBO末尾に
 * base_align未満のFREE rangeが残る場合がある。
 *
 * @par Allocation数上限
 * max_allocation_countは、このVBO Manager内で同時に生存できる
 * allocation数の上限を表す。VBO Managerの生存期間全体における
 * allocation実行回数の上限ではない。
 *
 * 現在のGLCEでは、一つのジオメトリーにつき、対応するshaderのVBO内に
 * 一つのrangeを確保する。このため、実用上のmax_allocation_countは、
 * 対象VBO内に同時に存在させるジオメトリー数を基準に決定できる。
 *
 * 0は指定できない。
 *
 * @par 固定base alignment
 * base_alignは、このVBO Managerが生成するすべてのallocationに
 * 共通して適用するalignmentをbyte単位で表す。
 *
 * 0以外の2の冪乗でなければならない。
 * allocationごとに異なるalignmentを指定することはできない。
 *
 * @par Buffer usage
 * buffer_usageは、GPU側VBOの生成時にRenderer Backendへ渡す
 * buffer使用方法のhintを表す。
 *
 * BUFFER_USAGE_STATICまたはBUFFER_USAGE_DYNAMICのいずれかを指定する。
 * この設定はRenderer BackendによるVBOの生成方法へ影響するが、
 * VBO Managerのallocationおよびfreeの動作は変更しない。
 *
 * @see vbo_manager_config_is_valid
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef struct vbo_manager_config {
    size_t vbo_size;                /**< GPU側VBOおよび論理memory poolの総サイズ(byte) */
    size_t max_allocation_count;    /**< 同時に生存できるallocation数の上限 */
    size_t base_align;              /**< 全allocationへ適用する固定base alignment(byte) */
    buffer_usage_t buffer_usage;    /**< Renderer Backendへ指定するbuffer usage */
} vbo_manager_config_t;

/**
 * @brief VBO Managerから取得したvertex allocationを表すhandle
 *
 * @details
 * vbo_manager_write()によって確保されたVBO内rangeの位置、実確保サイズ、
 * allocation identity、および生成元Range Allocatorを保持する。
 *
 * 現在はrange_allocation_tを内包し、VBO内rangeの管理を
 * Range Allocatorへ委譲している。
 *
 * @par Handleの使用
 * range_allocationのoffsetはVBO先頭からデータ転送開始位置までの
 * byte offsetを表し、allocated_sizeはbase alignment調整後に
 * Range Allocatorが占有している実確保サイズを表す。
 *
 * 本handleはvbo_manager_free()へ渡すまで変更してはならない。
 * 内部fieldを変更すると、対応するlive allocationを解決できなくなる。
 *
 * @par 生存期間
 * 本handleは、生成元VBO Managerが生存し、対応するallocationが
 * freeされるまで有効である。
 *
 * vbo_manager_free()はhandle自体を変更しない。
 * free成功後も各fieldの値は残るが、live allocationを表さなくなるため、
 * 再度vbo_manager_free()へ渡してはならない。
 *
 * VBO Managerを破棄した場合、そのVBO Managerから取得した
 * すべてのvertex_allocation_tは無効となる。
 *
 * @par Handleの複製
 * 本handleを値として複製しても、VBO内のallocationは複製されない。
 * 複数のhandleから同じallocationをfreeするとdouble freeになる。
 *
 * @par 将来拡張
 * 将来VBO poolまたはVBO pageを導入した場合に、対象VBOやpageを
 * 識別する情報を追加できるよう、range_allocation_tを公開APIで
 * 直接使用せず、独立したvertex_allocation_tとして定義する。
 *
 * @see range_allocation_t
 * @see vbo_manager_write
 * @see vbo_manager_free
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef struct vertex_allocation {
    range_allocation_t range_allocation;    /**< VBO内rangeのallocation descriptor */
} vertex_allocation_t;

/**
 * @brief VBO Managerの内部状態を保持するopaque型
 *
 * @details
 * 呼び出し側は内部メンバーへ直接アクセスせず、
 * VBO Managerの公開APIを通して本型を操作する。
 *
 * 内部では、生成時のconfig、VBO内rangeを管理するRange Allocator、
 * およびRenderer Backend VBOを保持する。
 *
 * インスタンスはvbo_manager_create()によって生成され、
 * vbo_manager_destroy()によって破棄される。
 *
 * @see vbo_manager_create
 * @see vbo_manager_destroy
 */
typedef struct vbo_manager vbo_manager_t;

typedef struct renderer_backend_context renderer_backend_context_t; /**< Renderer Backend Contextのopaque型 */

/**
 * @brief VBO Managerを生成する
 *
 * @details
 * 指定された生成設定を使用して、VBO Manager本体、VBO内rangeを管理する
 * Range Allocator、およびRenderer Backend VBOを生成する。
 *
 * GPU側VBOにはconfig_->vbo_sizeで指定されたbuffer領域を確保する。
 * buffer領域の初期内容は設定せず、その値は保証しない。
 *
 * config_の内容はVBO Manager内部へ値としてコピーする。
 * config_へのpointerは保持せず、その所有権も取得しない。
 *
 * @par Resource生成順序
 * 次の順序で内部リソースを生成する。
 *
 * 1. VBO Manager本体のメモリを確保する
 * 2. Range Allocatorを生成する
 * 3. Renderer Backend VBOを生成する
 * 4. 生成したVBOをbindする
 * 5. GPU側VBOのbuffer領域を確保する
 * 6. VBOをunbindする
 * 7. 完成したVBO Managerをout_vbo_manager_へ格納する
 *
 * @par Rollback
 * すべての生成処理が成功した場合にのみ、生成したVBO Managerを
 * out_vbo_manager_へ格納する。
 *
 * 途中で失敗した場合は、必要に応じてVBOをunbindした後、
 * 生成済みのRenderer Backend VBO、Range Allocator、および
 * VBO Manager本体を逆順に破棄する。
 *
 * rollback中に、正常なRenderer Backend Contextでは成功するはずの
 * VBOのunbindに失敗した場合は、BUFFER_MANAGER_DATA_CORRUPTEDを返す。
 *
 * 本関数は、呼び出し前にbindされていたVBOを保存および復元しない。
 * 生成処理でVBOのbindに成功した場合、成功時およびrollback完了時の
 * vertex buffer bindingはunbindされた状態となる。
 *
 * @pre Memory Systemが初期化されていること。
 * @pre backend_context_が有効なRenderer Backend Contextを指していること。
 * @pre vbo_manager_config_is_valid(config_)がtrueを返すこと。
 * @pre out_vbo_manager_が指すpointerがNULLであること。
 *
 * @post 成功時は、次の状態を持つVBO Managerを*out_vbo_manager_へ格納する。
 * - config_の内容がVBO Manager内部へコピーされている
 * - Range Allocatorの全rangeがFREEである
 * - live allocation数が0である
 * - GPU側VBOにconfig_->vbo_sizeのbuffer領域が確保されている
 * - vertex buffer bindingがunbindされた状態である
 *
 * @post 失敗時は、生成途中のリソースを破棄し、
 * out_vbo_manager_が指すpointerを変更しない。
 *
 * @param[in,out] backend_context_
 * Renderer Backend VBOの生成、bind、buffer領域の確保、unbind、および
 * rollback時の破棄に使用するRenderer Backend Context。
 * 本関数は所有権を取得せず、生成したVBO Manager内部にも保持しない。
 *
 * @param[in] config_
 * VBO Managerの生成設定。
 * vbo_manager_config_is_valid()がtrueを返す設定を指定する。
 * 成功時に内容がVBO Manager内部へコピーされる。
 *
 * @param[in,out] out_vbo_manager_
 * 生成したVBO Managerの格納先。
 * 有効なpointerを指定し、呼び出し前に*out_vbo_manager_をNULLにする必要がある。
 * 成功時のみ生成したインスタンスが格納される。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * VBO Managerの生成に成功した。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * 引数が無効、または下位モジュールが無効な引数を検出した。
 * 直接検証する条件には次が含まれる。
 * - backend_context_がNULL
 * - config_がNULL
 * - out_vbo_manager_がNULL
 * - *out_vbo_manager_がNULLではない
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * config_がVBO Managerの生成条件を満たしていない、
 * または下位モジュールの現在の状態では生成処理を実行できない。
 *
 * @retval BUFFER_MANAGER_LIMIT_EXCEEDED
 * Memory System、Range Allocator、またはRenderer Backendの
 * 管理上限に到達した。
 *
 * @retval BUFFER_MANAGER_NO_MEMORY
 * VBO Manager、Range Allocator、Renderer Backend VBO、または
 * GPU側buffer領域の生成に必要なメモリを確保できなかった。
 *
 * @retval BUFFER_MANAGER_RUNTIME_ERROR
 * Renderer BackendでVBOの生成、bind、buffer領域の確保、または
 * unbindに関する実行時エラーが発生した。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * 下位モジュールが内部状態の破損を検出した、
 * またはrollback中のVBO unbindに失敗した。
 *
 * @retval BUFFER_MANAGER_OVERFLOW
 * 下位モジュールにおける演算が表現可能範囲を超えた。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * 下位モジュールから未定義または変換対象外の結果コードを受け取った。
 *
 * @par 計算量
 * CPU側の時間計算量および追加memory使用量は、
 * Range Allocatorのnode pool生成によりO(config_->max_allocation_count)である。
 *
 * @see vbo_manager_config_t
 * @see vbo_manager_config_is_valid
 * @see vbo_manager_destroy
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
buffer_manager_result_t vbo_manager_create(renderer_backend_context_t* backend_context_, const vbo_manager_config_t* config_, vbo_manager_t** out_vbo_manager_);

/**
 * @brief VBO Managerを破棄する
 *
 * @details
 * VBO Managerが所有するRenderer Backend VBO、Range Allocator、
 * およびVBO Manager本体を解放し、*vbo_manager_へNULLを設定する。
 *
 * live allocationが存在する状態でもVBO Managerを破棄できる。
 * 破棄後は、このVBO Managerから取得したすべての
 * vertex_allocation_tが無効となる。
 *
 * backend_context_がNULLの場合は、エラーメッセージを出力し、
 * リソースを破棄せずに終了する。この場合、*vbo_manager_は変更されない。
 *
 * backend_context_がNULLではなく、vbo_manager_または
 * *vbo_manager_がNULLの場合は何も行わずに終了する。
 * したがって、有効なRenderer Backend Contextを指定して同じpointer変数から
 * 複数回呼び出した場合、2回目以降の呼び出しはno-opとなる。
 *
 * @pre backend_context_には、対象VBOを破棄できる有効な
 * Renderer Backend Contextを指定すること。
 * @pre VBO Managerの生存期間中はMemory Systemが初期化されていること。
 *
 * @param[in,out] vbo_manager_
 * 破棄するVBO Managerを保持するpointerの格納先。
 * 正常に破棄した後は*vbo_manager_へNULLが設定される。
 *
 * backend_context_が有効であり、vbo_manager_または
 * *vbo_manager_がNULLの場合は何も行わない。
 *
 * @param[in,out] backend_context_
 * Renderer Backend VBOの破棄に使用するRenderer Backend Context。
 * 対象VBOを生成したContext、またはそのVBOを破棄できるContextを指定する。
 * 本関数は所有権を取得しない。
 *
 * @post 有効なVBO ManagerとRenderer Backend Contextを指定し、
 * 下位モジュールが正常な場合、次の状態となる。
 * - Renderer Backend VBOが破棄されている
 * - Range Allocatorが破棄されている
 * - VBO Manager本体が解放されている
 * - *vbo_manager_がNULLである
 *
 * @warning
 * Renderer Backend VBOの破棄、Range Allocatorの破棄、および
 * VBO Manager本体のメモリ解放について、本関数は成否を取得できない。
 *
 * Memory SystemまたはRenderer Backend Contextの内部状態に問題がある場合、
 * リソースが正常に解放されない可能性があるが、本関数は
 * *vbo_manager_へNULLを設定する。
 *
 * @par 計算量
 * CPU側で管理するリソースは個別要素を走査せずに解放するため、
 * VBO ManagerおよびRange Allocatorの破棄に関する時間計算量はO(1)である。
 * Renderer BackendおよびGPU driver内部の処理量は実装に依存する。
 *
 * @see vbo_manager_create
 * @see vbo_manager_free
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void vbo_manager_destroy(vbo_manager_t** vbo_manager_, renderer_backend_context_t* backend_context_);

/**
 * @brief VBO内にrangeを確保してvertex dataを転送する
 *
 * @details
 * Range Allocatorからsize_と同じサイズのrangeを確保し、
 * 対応するRenderer Backend VBOをbindした後、確保したrangeのoffsetへ
 * write_data_の内容を転送する。
 *
 * size_はVBO Managerのbase alignmentの倍数でなければならない。
 * このため、Range Allocatorによるalignment paddingは発生せず、
 * allocationの実確保サイズとGPU側VBOへ転送するサイズは
 * いずれもsize_となる。
 *
 * VBO Managerは、write_data_が完全なvertexまたはelement単位で
 * 構成されているか検証しない。この条件は上位層が保証する。
 *
 * 転送成功後にVBOをunbindし、確保したrangeを表す
 * vertex_allocation_tをout_allocation_handle_へ格納する。
 *
 * @par 処理順序
 * 次の順序でrangeの確保とデータ転送を行う。
 *
 * 1. Range Allocatorからsize_と同じサイズのrangeを確保する
 * 2. 対象VBOをbindする
 * 3. 確保したrangeのoffsetへsize_ byteのデータを転送する
 * 4. VBOをunbindする
 * 5. allocation handleをout_allocation_handle_へ格納する
 *
 * @par Rollback
 * range確保後にVBOのbind、データ転送、またはunbindへ失敗した場合は、
 * 確保したrangeをRange Allocatorへ返却する。
 *
 * VBOがbindされた状態で失敗した場合は、rangeの返却後に
 * VBOのunbindを再試行する。
 *
 * rollback中のrange解放またはVBO unbindに失敗した場合は、
 * 内部状態を保証できないためBUFFER_MANAGER_DATA_CORRUPTEDを返す。
 *
 * データ転送処理が部分的または完全に実行された後で失敗した場合でも、
 * GPU側VBOに書き込まれたデータの復元または消去は行わない。
 * rollbackに成功したrangeは後続のwriteによる再利用対象となる。
 *
 * allocation handleは処理全体が成功した場合にのみ更新される。
 *
 * @pre vbo_manager_がvbo_manager_create()によって正常に生成され、
 *      破棄されていないこと。
 * @pre backend_context_が対象VBOを操作できる有効な
 *      Renderer Backend Contextを指していること。
 * @pre write_data_が少なくともsize_ byteの読み取り可能な領域を
 *      指していること。
 *
 * @param[in,out] vbo_manager_
 * データを書き込むVBO Manager。
 * 成功時は内部Range Allocatorのallocation状態が更新される。
 *
 * @param[in] backend_context_
 * VBOのbind、データ転送、およびunbindに使用する
 * Renderer Backend Context。
 * 本関数は所有権を取得せず、VBO Manager内部にも保持しない。
 *
 * @param[in] size_
 * GPU側VBOへ転送するデータサイズ(byte)。
 * 0は指定できず、VBO Managerのbase alignmentの倍数でなければならない。
 *
 * 正常な頂点データでは、上位層によって
 * sizeof(vertex_t)とvertex数の積として計算される。
 *
 * @param[in] write_data_
 * GPU側VBOへ転送するデータの先頭pointer。
 * 少なくともsize_ byteを読み取り可能でなければならない。
 * 本関数は所有権を取得しない。
 *
 * @param[out] out_allocation_handle_
 * 確保したVBO内rangeを表すallocation handleの格納先。
 * 成功時のみrange_allocationが設定され、失敗時は変更されない。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * rangeの確保、データ転送、VBOのunbind、および
 * allocation handleの生成に成功した。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * 引数が無効、または下位モジュールが無効な引数を検出した。
 * 直接検証する条件には次が含まれる。
 * - vbo_manager_がNULL
 * - backend_context_がNULL
 * - size_が0
 * - write_data_がNULL
 * - out_allocation_handle_がNULL
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * 次のいずれか。
 * - size_がVBO Managerのbase alignmentの倍数ではない
 * - Range AllocatorまたはRenderer Backendの現在の状態では
 *   要求された処理を実行できない
 *
 * @retval BUFFER_MANAGER_LIMIT_EXCEEDED
 * VBO Manager内のlive allocation数が
 * config.max_allocation_countへ到達した、
 * または下位モジュールの管理上限に到達した。
 *
 * @retval BUFFER_MANAGER_NO_MEMORY
 * size_を収容できる連続したFREE rangeがVBO内に存在しない、
 * またはRenderer Backendが必要なmemoryを確保できない。
 *
 * VBO内のtotal free sizeがsize_以上でも、断片化によって
 * 十分な大きさの連続FREE rangeが存在しない場合はこの結果となる。
 *
 * @retval BUFFER_MANAGER_RUNTIME_ERROR
 * Renderer BackendでVBOのbind、データ転送、または
 * unbindに関する実行時エラーが発生した。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * 次のいずれか。
 * - VBO Managerが基本的な内部不変条件を満たしていない
 * - Range AllocatorまたはRenderer Backendが内部状態の破損を検出した
 * - rollback中のrange解放に失敗した
 * - rollback中のVBO unbindに失敗した
 *
 * @retval BUFFER_MANAGER_OVERFLOW
 * range offset、range終端、またはその他の下位モジュールにおける
 * size_t演算が表現可能範囲を超えた。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * 下位モジュールから未定義または変換対象外の結果コードを受け取った。
 *
 * @post 成功時は次が成立する。
 * - out_allocation_handle_はlive allocationを表す
 * - range_allocation.allocated_sizeはsize_と一致する
 * - range_allocation.offsetはbase alignment境界にある
 * - VBO Managerのlive allocation数が1増加する
 * - write_data_の先頭size_ byteが確保したrange全体へ転送されている
 * - vertex buffer bindingがunbindされた状態である
 *
 * @post 失敗時はout_allocation_handle_を変更しない。
 *
 * @post range確保後の失敗に対するrollbackが成功した場合、
 *       Range Allocatorの論理的なallocation状態は呼び出し前と等しくなる。
 *
 * @warning
 * out_allocation_handle_がすでにlive allocationを表している場合、
 * 成功時に既存のhandleが上書きされる。
 * 既存allocationを先にfreeするか、別のhandleへ保存してから
 * 呼び出す必要がある。
 *
 * @warning
 * rollbackに失敗してBUFFER_MANAGER_DATA_CORRUPTEDを返した場合、
 * Range AllocatorまたはVBOのbinding状態が呼び出し前と
 * 異なる可能性がある。
 *
 * @par 計算量
 * VBO Manager自身の制御処理はO(1)である。
 * range確保の時間計算量はrange_allocator_allocate()に依存し、
 * データ転送の処理量はRenderer BackendおよびGPU driverの実装に依存する。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_free
 * @see vertex_allocation_t
 * @see range_allocator_allocate
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
buffer_manager_result_t vbo_manager_write(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_, size_t size_, const void* write_data_, vertex_allocation_t* out_allocation_handle_);

/**
 * @brief vertex allocationが表すVBO内rangeを解放する
 *
 * @details
 * allocation_handle_に内包されたrange_allocation_tを使用して、
 * 対応するlive allocationをVBO ManagerのRange Allocatorから解放する。
 *
 * allocation handleのowner、node index、offset、およびallocated sizeは
 * Range Allocatorによって検証される。
 *
 * 解放するrangeに隣接するFREE rangeが存在する場合は、
 * Range Allocatorによって一つのFREE rangeへmergeされる。
 *
 * 本関数はGPU側VBOへアクセスせず、解放したrangeに保存されている
 * データの消去またはゼロクリアを行わない。
 * 解放されたrangeは後続のvbo_manager_write()による再利用対象となる。
 *
 * allocation_handle_自体は変更しない。
 * 成功後も各fieldの値は残るが、live allocationを表さなくなるため、
 * 再度本関数へ渡してはならない。
 *
 * @par Free保証
 * VBO ManagerとRange Allocatorの内部状態が正常であり、
 * vbo_manager_write()が返した変更されていないlive allocation handleを、
 * 生成元のVBO Managerへ渡した場合、本関数は成功する。
 *
 * freeでは新しいnodeの取得、動的メモリ確保、および
 * 動的メモリ解放を行わない。
 * したがって、validなfreeがnode不足またはメモリ不足を理由として
 * 失敗することはない。
 *
 * @pre vbo_manager_がvbo_manager_create()によって正常に生成され、
 *      破棄されていないこと。
 * @pre allocation_handle_がvbo_manager_によって生成された、
 *      変更されていないlive allocationを表していること。
 *
 * @param[in,out] vbo_manager_
 * allocationを解放するVBO Manager。
 * 成功時は内部Range Allocatorのrange list、node pool、
 * allocation count、およびtotal allocated sizeが更新される。
 *
 * @param[in] allocation_handle_
 * 解放するVBO内rangeを表すallocation handle。
 *
 * 本関数はhandle自体を変更しない。
 * 成功後も各fieldの値は残るが、live allocationを表さなくなる。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * allocationの解放と、必要な隣接FREE rangeのmergeに成功した。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * 次のいずれか。
 * - vbo_manager_がNULL
 * - allocation_handle_がNULL
 * - 下位モジュールが無効な引数を検出した
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * allocation handleがlive allocationを正しく表していない。
 * 次のような場合が該当する。
 * - range_allocation.allocated_sizeが0
 * - allocation handleのownerがvbo_manager_のRange Allocatorと一致しない
 * - VBO Managerにlive allocationが存在しない
 * - node indexがnode poolの範囲外である
 * - 対応nodeがALLOCATED状態ではない
 * - handleのoffsetまたはallocated sizeが対応nodeと一致しない
 * - 解放済みhandleを再度指定した
 * - allocation handleのfieldが変更されている
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * 次のいずれか。
 * - VBO Managerが基本的な内部不変条件を満たしていない
 * - Range Allocatorが内部不変条件を満たしていない
 * - rangeまたはnodeの接続関係に不整合がある
 * - FREE rangeのmergeまたはnodeの返却に失敗した
 *
 * @post 成功時は次が成立する。
 * - allocation_handle_が表していたrangeがFREE rangeへ統合される
 * - VBO Managerのlive allocation数が1減少する
 * - total allocated sizeがrange_allocation.allocated_sizeだけ減少する
 * - allocation_handle_自体の内容は変更されない
 * - allocation_handle_はlive allocationを表さなくなる
 * - GPU側VBOの保存内容およびbinding状態は変更されない
 *
 * @post 内部状態の変更を開始する前に失敗した場合、
 *       VBO Manager、Range Allocator、およびallocation_handle_の
 *       内容は変更されない。
 *
 * @warning
 * Range Allocatorがrange listの変更を開始した後に
 * BUFFER_MANAGER_DATA_CORRUPTEDが発生した場合、内部状態を
 * 呼び出し前へrollbackできない可能性がある。
 *
 * @warning
 * allocation handleはnode generationを保持しない。
 * 解放済みnodeが別のallocationへ再利用され、owner、node index、
 * offset、およびallocated sizeがすべて一致した場合、
 * stale handleを検出できない。
 *
 * 解放成功後のallocation handleは再利用せず、
 * 必要に応じて呼び出し側でゼロ初期化すること。
 *
 * @note
 * validなallocationの解放では動的リソースを新たに取得しないため、
 * 本関数はBUFFER_MANAGER_NO_MEMORYおよび
 * BUFFER_MANAGER_LIMIT_EXCEEDEDを返さない。
 *
 * GPU側VBOへアクセスしないため、
 * BUFFER_MANAGER_RUNTIME_ERRORも返さない。
 *
 * @par 計算量
 * VBO Manager自身の制御処理はO(1)である。
 * range解放の時間計算量はrange_allocator_free()に依存する。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_write
 * @see vertex_allocation_t
 * @see range_allocator_free
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
buffer_manager_result_t vbo_manager_free(vbo_manager_t* vbo_manager_, const vertex_allocation_t* allocation_handle_);

/**
 * @brief VBO Managerが所有するRenderer Backend VBOをbindする
 *
 * @details
 * vbo_manager_が所有するRenderer Backend VBOを、
 * backend_context_の現在のvertex buffer bindingへ設定する。
 *
 * 呼び出し前に別のVBOがbindされていた場合、そのbindingは
 * vbo_manager_が所有するVBOへ置き換えられる。
 * 以前bindされていたVBOの保存および自動的な復元は行わない。
 *
 * 本関数はVBO Managerのconfig、Range Allocatorのallocation状態、
 * およびGPU側VBOの保存内容を変更しない。
 *
 * VBO Manager自身はbinding状態を保持しないため、
 * 必要な処理の完了後に呼び出し側がvbo_manager_unbind()を実行する。
 *
 * @pre vbo_manager_がvbo_manager_create()によって正常に生成され、
 *      破棄されていないこと。
 * @pre backend_context_がvbo_manager_のRenderer Backend VBOを操作できる
 *      有効なRenderer Backend Contextを指していること。
 *
 * @param[in] vbo_manager_
 * bindするRenderer Backend VBOを所有するVBO Manager。
 * 本関数はVBO Managerの内部状態を変更しない。
 *
 * @param[in] backend_context_
 * VBOのbindに使用するRenderer Backend Context。
 * 本関数は所有権を取得せず、VBO Manager内部にも保持しない。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * VBO Managerが所有するVBOのbindに成功した。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * 次のいずれか。
 * - vbo_manager_がNULL
 * - backend_context_がNULL
 * - Renderer Backendが無効な引数を検出した
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * Renderer Backend ContextがVBO操作用に初期化されていない、
 * Renderer Backend VBOが有効ではない、または現在の状態では
 * bindを実行できない。
 *
 * @retval BUFFER_MANAGER_RUNTIME_ERROR
 * Renderer BackendでVBOのbindに関する実行時エラーが発生した。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * VBO Managerが基本的な内部不変条件を満たしていない、
 * またはRenderer Backendが内部状態の破損を検出した。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * Renderer Backendから未定義または変換対象外の結果コードを受け取った。
 *
 * @post 成功時は、vbo_manager_が所有するRenderer Backend VBOが
 *       現在のvertex buffer bindingとなる。
 *
 * @post 成功時も、VBO Managerのconfig、Range Allocatorの状態、
 *       およびGPU側VBOの保存内容は変更されない。
 *
 * @par 計算量
 * VBO Manager自身の制御処理はO(1)である。
 * Renderer BackendおよびGPU driver内部の処理量は実装に依存する。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_unbind
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
buffer_manager_result_t vbo_manager_bind(vbo_manager_t* vbo_manager_, const renderer_backend_context_t* backend_context_);

/**
 * @brief 現在のRenderer Backend VBO bindingを解除する
 *
 * @details
 * backend_context_における現在のvertex buffer bindingを解除する。
 *
 * 本関数は特定のVBO ManagerまたはRenderer Backend VBOを指定して
 * unbindするものではない。現在bindされているVBOがVBO Managerの
 * 所有物ではない場合でも、そのbindingを解除する。
 *
 * VBOがbindされていない状態で呼び出した場合の動作は
 * Renderer Backendの契約に従う。現在のOpenGL Backendでは、
 * unbind状態を再設定して成功する。
 *
 * 本関数はRenderer Backend VBOを破棄せず、GPU側VBOの保存内容、
 * VBO Managerのconfig、およびRange Allocatorのallocation状態を
 * 変更しない。
 *
 * @pre backend_context_がVBO操作を実行できる有効な
 *      Renderer Backend Contextを指していること。
 *
 * @param[in] backend_context_
 * 現在のvertex buffer bindingを解除するために使用する
 * Renderer Backend Context。
 * 本関数は所有権を取得しない。
 *
 * @retval BUFFER_MANAGER_SUCCESS
 * 現在のvertex buffer bindingの解除に成功した。
 *
 * @retval BUFFER_MANAGER_INVALID_ARGUMENT
 * backend_context_がNULL、またはRenderer Backendが
 * 無効な引数を検出した。
 *
 * @retval BUFFER_MANAGER_BAD_OPERATION
 * Renderer Backend ContextがVBO操作用に初期化されていない、
 * または現在の状態ではunbindを実行できない。
 *
 * @retval BUFFER_MANAGER_RUNTIME_ERROR
 * Renderer BackendでVBOのunbindに関する実行時エラーが発生した。
 *
 * @retval BUFFER_MANAGER_DATA_CORRUPTED
 * Renderer Backendが内部状態の破損を検出した。
 *
 * @retval BUFFER_MANAGER_UNDEFINED_ERROR
 * Renderer Backendから未定義または変換対象外の結果コードを受け取った。
 *
 * @post 成功時は、backend_context_における現在の
 *       vertex buffer bindingが解除されている。
 *
 * @post 成功時も、Renderer Backend VBO、GPU側VBOの保存内容、
 *       VBO Manager、およびRange Allocatorの状態は変更されない。
 *
 * @par 計算量
 * VBO Manager側の制御処理はO(1)である。
 * Renderer BackendおよびGPU driver内部の処理量は実装に依存する。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_bind
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
buffer_manager_result_t vbo_manager_unbind(const renderer_backend_context_t* backend_context_);

/**
 * @brief VBO Manager生成設定の基本的な有効性を検証する
 *
 * @details
 * config_に設定されたVBO size、allocation数上限、固定base alignment、
 * およびbuffer usageが、VBO Managerの基本的な生成条件を満たしているか検証する。
 *
 * 本関数はconfig_を参照するだけであり、内容を変更しない。
 * また、動的メモリの確保、Range Allocatorの生成、および
 * Renderer BackendやGPU側VBOへのアクセスは行わない。
 *
 * 本関数がtrueを返しても、Memory SystemやRenderer Backendの状態、
 * 動的メモリの確保可否、および下位処理で行われるsize_t演算の結果などにより、
 * vbo_manager_create()が成功することは保証されない。
 *
 * @param[in] config_
 * 検証対象のVBO Manager生成設定。
 * NULLを指定した場合はfalseを返す。
 *
 * @retval true
 * 次のすべてが成立している。
 * - config_がNULLではない
 * - vbo_sizeが0ではない
 * - max_allocation_countが0ではない
 * - base_alignが0以外の2の冪乗である
 * - buffer_usageがBUFFER_USAGE_STATICまたはBUFFER_USAGE_DYNAMICである
 *
 * @retval false
 * 上記条件のいずれかが成立していない。
 *
 * @par 計算量
 * 時間計算量はO(1)である。
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_config_t
 * @see vbo_manager_create
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
bool vbo_manager_config_is_valid(const vbo_manager_config_t* config_);

/**
 * @brief VBO Managerの基本状態を標準出力へ表示する
 *
 * @details
 * VBO Managerのshallow validation結果、生成設定、および
 * Range Allocatorの状態snapshotを標準出力へ表示する。
 *
 * VBO Managerが基本的な内部不変条件を満たしている場合は、
 * 次の情報を表示する。
 *
 * - vbo_manager_is_valid()の結果
 * - VBO全体のサイズ
 * - 同時に生存できるallocation数の上限
 * - buffer usage
 * - Range Allocatorの容量、base alignment、node使用状況、
 *   allocation数、および使用量
 *
 * vbo_manager_がNULL、または基本的な内部不変条件を満たしていない場合は、
 * validation結果がfalseであることだけを表示し、
 * VBO Manager内部およびRange Allocatorの状態を参照しない。
 *
 * 本関数が行うVBO Managerの検証はshallow validationである。
 * Range Allocatorのrange listおよびnode pool、Renderer Backend VBO、
 * ならびに実際のGPU側VBOに対するdeep validationは行わない。
 *
 * 本関数はVBO Manager、Range Allocator、Renderer Backend VBO、
 * およびGPU側VBOの状態を変更しない。
 *
 * @param[in] vbo_manager_
 * 状態を表示するVBO Manager。
 * NULLを指定した場合は、validation結果がfalseであることを表示する。
 *
 * NULLではない場合は、読み取り可能なVBO Managerを指していなければならない。
 * 破棄済みのdangling pointerは指定できない。
 *
 * @note
 * 出力先はstdoutであり、見出しにはANSI escape sequenceによる
 * 色指定を使用する。
 *
 * VBO Manager自身の状態表示とRange Allocatorの状態表示は、
 * それぞれ標準出力のstream lockを使用して出力される。
 *
 * @warning
 * 本関数はdiagnostic表示用であり、VBO ManagerまたはRange Allocatorの
 * 正常性を保証するvalidation APIではない。
 *
 * また、他のthreadが同じVBO Managerへallocateまたはfreeを実行している状態で
 * 呼び出すことを想定していない。必要な排他制御は呼び出し側が行う。
 *
 * @par 計算量
 * VBO Managerのshallow validationおよびRange Allocatorの
 * status取得はいずれもO(1)であるため、本関数の時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_debug_print
 * @see range_allocator_status_get
 * @see range_allocator_status_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void vbo_manager_status_print(const vbo_manager_t* vbo_manager_);

/**
 * @brief VBO Managerの内部状態を標準出力へdebug表示する
 *
 * @details
 * VBO Managerのshallow validation結果と生成設定をstdoutへ表示する。
 *
 * VBO Managerが基本的な内部不変条件を満たしている場合は、
 * 所有するRange Allocatorのdebug表示を
 * range_allocator_debug_print()へ委譲する。
 *
 * VBO Managerについて、次の情報を表示する。
 *
 * - vbo_manager_is_valid()の結果
 * - VBO全体のサイズ
 * - 同時に生存できるallocation数の上限
 * - buffer usage
 *
 * Range Allocatorが表示する情報およびvalidation範囲の詳細は、
 * range_allocator_debug_print()の契約に従う。
 *
 * vbo_manager_がNULL、または基本的な内部不変条件を満たしていない場合は、
 * validation結果がfalseであることだけを表示し、
 * Range Allocatorのdebug表示は行わない。
 *
 * Renderer Backend VBOおよび実際のGPU側VBOの状態は検証しない。
 *
 * @param[in] vbo_manager_
 * debug表示するVBO Manager。
 * NULLを指定した場合は、validation結果がfalseであることを表示する。
 *
 * NULLではない場合は、読み取り可能なVBO Managerを指していなければならない。
 * 破棄済みのdangling pointerは指定できない。
 *
 * @post VBO Manager、Range Allocator、Renderer Backend VBO、
 *       およびGPU側VBOの状態は変更されない。
 *
 * @note
 * 出力先はstdoutであり、見出しにはANSI escape sequenceによる
 * 色指定を使用する。
 *
 * fprintf()によるstdoutへの書き込み失敗は呼び出し側へ通知しない。
 *
 * @warning
 * 本関数はVBO ManagerおよびRange Allocatorに対する排他制御を行わない。
 * 必要な同期は呼び出し側が行う。
 *
 * @note
 * 本関数はdiagnostic表示用であり、VBO Manager全体、
 * Renderer Backend VBO、またはGPU側VBOの正常性を保証する
 * validation APIではない。
 *
 * @par 計算量
 * VBO Manager自身の処理はO(1)である。
 * 関数全体の時間計算量はrange_allocator_debug_print()に依存する。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see vbo_manager_status_print
 * @see range_allocator_debug_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void vbo_manager_debug_print(const vbo_manager_t* vbo_manager_);

#ifdef __cplusplus
}
#endif
#endif
