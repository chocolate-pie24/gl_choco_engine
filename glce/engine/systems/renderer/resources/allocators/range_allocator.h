/**
 * @file range_allocator.h
 * @brief GPU buffer内の固定alignment rangeを管理するRange Allocatorの操作API
 *
 * @details
 * Range Allocatorは、メモリプール内の連続rangeをbyte単位のoffsetとsizeで管理する。
 * 実メモリ、base pointer、またはGPU buffer object自体は所有せず、
 * offset 0を起点とする論理的なrangeだけを管理する。
 *
 * 現在は、OpenGL backendで使用するVBO／EBO内のrange管理を主な用途とする。
 *
 * @par Range list
 * メモリプール全体を、address orderで接続されたFREE／ALLOCATED nodeによって
 * 隙間なく表現する。
 *
 * allocation成功時には、対応するALLOCATED nodeのidentity、offset、および
 * alignment調整後の実確保サイズをrange_allocation_tとして呼び出し側へ返す。
 *
 * @par 固定alignment
 * Range Allocatorごとに、create時にbase alignmentを一つだけ設定する。
 * allocationごとに異なるalignmentを扱わず、range_allocator_allocate()へ
 * 指定するrequired alignmentはbase alignmentと一致しなければならない。
 *
 * GLCEではshaderごとに専用のVBO／EBOとRange Allocatorを持たせる。
 * 同じRange Allocatorが管理するbuffer内に、異なるshaderや異なるvertex layoutに
 * 由来する複数のalignment要件を混在させない。
 * このため、現在の用途では一つのRange Allocator内でallocationごとに
 * alignmentを変更する必要がない。
 *
 * 固定alignment方式を採用することで、offset調整、paddingを含む実確保サイズ、
 * およびfree対象rangeを単純かつ一意に扱える。
 * allocationごとにalignmentを変更できる方式と比べて、range探索、分割、
 * descriptor管理、およびfree処理の複雑化を避けることを優先している。
 *
 * allocation開始offsetはbase alignment境界に整列し、ALLOCATED rangeの
 * block sizeはbase alignmentの倍数となる。
 * memory pool size自体にはbase alignmentの倍数であることを要求しないため、
 * memory pool末尾のFREE rangeにはbase alignment未満の端数が含まれる場合がある。
 *
 * Range Allocatorは実際のbase addressを保持しない。
 * 実メモリへ適用する場合のbase address側のalignmentは、呼び出し側が保証する。
 *
 * @par Allocation方式
 * FREE rangeの検索にはfirst-fit方式を使用する。
 * range listを先頭からaddress orderで走査し、必要な実確保サイズを収容できる
 * 最初のFREE nodeを選択する。
 *
 * best-fitなどの方式による断片化の最適化よりも、探索規則と実装が単純で、
 * allocation結果を理解しやすいことを優先している。
 * 断片化への追加対策は、実際に必要となった段階で上位のbuffer pool／page構成、
 * またはallocation方式の変更として検討する。
 *
 * @par Allocation数上限
 * max allocation countは、このRange Allocator内で同時に生存できる
 * allocation数の上限を表す。
 *
 * 現在のGLCEでは、一つのジオメトリーにつき、対応するshaderの各VBO／EBO内に
 * 一つのrangeを確保する。
 * したがって、実用上のmax allocation countは、そのVBO／EBO内に
 * 同時に存在させるジオメトリー数を基準に決定できる。
 *
 * @par Node pool
 * create時に、max allocation countから次の式で最大node数を決定し、
 * node pool全体を事前確保する。
 *
 * @code{.c}
 * max_node_count = max_allocation_count * 2 + 1;
 * @endcode
 *
 * ALLOCATED rangeとFREE rangeが交互に並び、両端にFREE rangeが存在する配置が、
 * 一定のallocation数に対して最も多くnodeを使用する。
 * 上記の式は、この最悪配置を表現するために必要なnode数である。
 *
 * create成功後のallocate／freeでは動的メモリ確保を行わない。
 * allocation数がmax allocation count未満で内部状態が正常であれば、
 * allocationの分割に必要なnodeを確保できる。
 *
 * @par Free保証とrollback
 * 本モジュールではFREE rangeだけでなく、ALLOCATED rangeもnodeとして
 * range list上で管理する。
 *
 * allocate成功時に対応するALLOCATED nodeを確保し、後続のfreeに必要な
 * range情報とidentityを保持する。
 * freeではこのALLOCATED nodeをFREEへ遷移させ、隣接するFREE rangeがあれば
 * mergeする。freeのために新しいnodeを取得したり、動的メモリを確保したりしない。
 *
 * 内部データが破損しておらず、range_allocator_allocate()が返した
 * range_allocation_tを変更、二重解放、または異なるownerへの指定をせずに
 * range_allocator_free()へ渡した場合、freeは成功する。
 *
 * これにより、Range Allocatorのallocate成功後に上位処理が失敗した場合、
 * 取得済みrangeを確実にfreeし、allocate前の状態へrollbackできる。
 *
 * validなfreeの失敗は、通常のnode不足やメモリ不足ではなく、
 * 不正なdescriptor、APIの誤用、または内部データ破損を示す。
 *
 * @par 適用範囲
 * 本モジュールは、offset管理が必要なVBO／EBOなどのGPU buffer内range管理を
 * 主な対象とする。
 *
 * freeには、allocateが返したoffset、実確保サイズ、およびnode identityを保持する
 * range_allocation_tが必要となる。
 * CPU側メモリ確保では、実ポインタを返して通常のfree操作を行えるallocatorの方が
 * 扱いやすいため、本モジュールをCPU側メモリリソースの確保には使用しない。
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
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_ALLOCATORS_RANGE_ALLOCATOR_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_ALLOCATORS_RANGE_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "engine/systems/renderer/resources/allocators/core/range_allocator_types.h"

/**
 * @brief Range Allocatorの状態snapshot
 *
 * @details
 * Range Allocatorの容量、alignment、node使用状況、およびallocation状況を保持する。
 *
 * range_allocator_status_get()によって取得した時点の値であり、
 * 取得後に行われたallocate／freeは本構造体へ自動的に反映されない。
 *
 * total_allocated_sizeとallocation_countはRange Allocatorが保持する管理値である。
 *
 * total_free_size、used_node_count、およびfree_block_countは、
 * Range Allocatorが保持する管理値からO(1)で導出される。
 * range listの走査や再集計は行われない。
 *
 * @note
 * total_free_sizeはすべてのFREE rangeの合計サイズであり、
 * 最大の連続FREE rangeサイズではない。
 * そのため、total_free_sizeだけでは特定サイズのallocationが
 * 成功するかどうかを判断できない。
 *
 * @note
 * 内部データが破損して管理値に矛盾がある場合、size_tのunderflowを避けるため、
 * total_free_size、used_node_count、またはfree_block_countが0へ
 * 飽和される場合がある。
 *
 * @see range_allocator_status_get
 * @see range_allocator_status_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef struct range_allocator_status {
    size_t memory_pool_size;        /**< Range Allocatorが管理する論理メモリプールの総容量(byte) */
    size_t base_align;              /**< allocationに使用する固定base alignment(byte) */

    size_t max_node_count;          /**< node poolに事前確保されたnodeの総数 */
    size_t max_allocation_count;    /**< 同時に生存できるallocation数の上限 */
    size_t total_allocated_size;    /**< alignment paddingを含む、全ALLOCATED rangeの合計サイズ(byte) */
    size_t total_free_size;         /**< memory_pool_sizeからtotal_allocated_sizeを差し引いた全FREE rangeの合計サイズ(byte) */

    size_t unused_node_count;       /**< node pool内でNOT_USED状態にあるnode数 */
    size_t free_block_count;        /**< range list上でFREE rangeを表しているnode数 */

    size_t used_node_count;         /**< node pool内でrange listの管理に使用されているnode数
                                     *
                                     *   内部状態が正常であれば、FREE node数とALLOCATED node数の合計となる。
                                     */

    size_t allocation_count;        /**< 現在生存しているallocation数 */
} range_allocator_status_t;

/**
 * @brief Range Allocatorを生成する
 *
 * @details
 * 指定された論理メモリプール容量、最大allocation数、および固定base alignmentを使用してRange Allocatorを生成する。
 *
 * 本関数が確保するのはRange Allocator本体とnode poolだけであり、memory_pool_size_で指定された容量の実メモリやGPU bufferは確保しない。
 *
 * node poolのnode数は次の式で決定する。
 *
 * @code{.c}
 * max_node_count = max_allocation_count_ * 2 + 1;
 * @endcode
 *
 * @pre Memory Systemが初期化されていること。
 * @pre out_range_allocator_が指すpointerがNULLであること。
 *
 * @post 成功時は、次の初期状態を持つRange Allocatorを*out_range_allocator_へ格納する。
 * - memory pool全体を表す一つのFREE nodeがrange listへ接続されている。
 * - range list先頭nodeのoffsetは0である。
 * - range list先頭nodeのblock sizeはmemory_pool_size_である。
 * - allocation countおよびtotal allocated sizeは0である。
 * - 先頭node以外のnodeはすべてNOT_USED状態である。
 *
 * @post 失敗時は、生成途中に確保したRange Allocator本体およびnode poolを解放し、out_range_allocator_が指すpointerを変更しない。
 *
 * @param[in] memory_pool_size_
 * 管理対象となる論理メモリプールの総容量(byte)。0は指定できない。
 * base_align_の倍数である必要はない。
 *
 * @param[in] max_allocation_count_
 * 同時に生存できるallocation数の上限。0は指定できない。
 *
 * @param[in] base_align_
 * 本Range Allocatorが使用する固定base alignment(byte)。
 * 0以外の2の冪乗でなければならない。
 *
 * @param[in,out] out_range_allocator_
 * 生成したRange Allocatorの格納先。
 * 有効なpointerを指定し、呼び出し前に*out_range_allocator_をNULLにする必要がある。
 * 成功時のみ生成したインスタンスが格納される。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * Range Allocatorの生成に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - out_range_allocator_がNULL
 * - *out_range_allocator_がNULLではない
 * - memory_pool_size_が0
 * - max_allocation_count_が0
 * - base_align_が0
 * - memory_system_allocate()がMEMORY_SYSTEM_INVALID_ARGUMENTを返した
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - base_align_が2の冪乗ではない
 * - Memory Systemが初期化されていない
 * - memory_system_allocate()がMEMORY_SYSTEM_BAD_OPERATIONを返した
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - max_allocation_count_ * 2 + 1がsize_tの表現可能範囲を超える
 * - sizeof(node_t) * max_node_countがsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * Range Allocator本体またはnode poolの確保によって、
 * Memory Systemのメモリタグ別使用量または総使用量がsize_tの表現可能範囲を超える。
 *
 * @retval RANGE_ALLOCATOR_NO_MEMORY
 * Range Allocator本体またはnode poolの動的メモリ確保に失敗した。
 *
 * @retval RANGE_ALLOCATOR_UNDEFINED_ERROR
 * memory_system_allocate()から変換対象外の結果コードを受け取った。
 *
 * @par 計算量
 * max_node_count個のnodeを初期化するため、時間計算量は
 * O(max_allocation_count_)である。
 *
 * @see range_allocator_destroy
 * @see memory_system_allocate
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
range_allocator_result_t range_allocator_create(size_t memory_pool_size_, size_t max_allocation_count_, size_t base_align_, range_allocator_t** out_range_allocator_);

/**
 * @brief Range Allocatorを破棄する
 *
 * @details
 * Range Allocatorが所有するnode poolと内部状態管理構造体を解放し、*range_allocator_へNULLを設定する。
 *
 * 管理対象となる実メモリ、GPU buffer object、およびその保存内容は
 * Range Allocatorの所有物ではないため、本関数では解放しない。
 *
 * range_allocator_がNULL、または*range_allocator_がNULLの場合は何も行わずに終了する。
 * したがって、同じpointer変数を使用した複数回の呼び出しはno-opとなる。
 *
 * 本関数はallocation countが0であることを要求せず、live allocationが存在する状態でもRange Allocatorを破棄する。
 * 破棄後は、このRange Allocatorをownerとして保持するすべてのrange_allocation_tが無効となる。
 *
 * @param[in,out] range_allocator_
 * 破棄するRange Allocatorを保持するpointerの格納先。
 * 正常に破棄した後は*range_allocator_へNULLが設定される。
 * range_allocator_または*range_allocator_がNULLの場合は何も行わない。
 *
 * @post 有効なRange Allocatorを指定し、Memory Systemが正常な場合、
 *       node poolとRange Allocator本体が解放され、*range_allocator_がNULLとなる。
 *
 * @warning
 * memory_system_free()は、Memory Systemが未初期化の場合や
 * メモリ使用量管理値に矛盾がある場合、対象メモリを解放せずに終了する。
 *
 * 本関数はmemory_system_free()の成否を取得できないため、その場合でも
 * *range_allocator_へNULLを設定する。
 * Range Allocatorの生存期間中はMemory Systemを破棄せず、
 * メモリ使用量管理値を正常に維持する必要がある。
 *
 * @par 計算量
 * node poolの各要素を走査せずに一括解放するため、時間計算量はO(1)である。
 *
 * @see range_allocator_create
 * @see memory_system_free
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void range_allocator_destroy(range_allocator_t** range_allocator_);

/**
 * @brief 固定alignmentで論理メモリプール内のrangeを確保する
 *
 * @details
 * required_size_をRange Allocatorのbase alignment単位に切り上げ、
 * alignment paddingを含むallocation sizeを求める。
 *
 * range listを先頭からfirst-fit方式で探索し、allocation sizeを収容できる
 * 最初のFREE rangeの先頭からrangeを確保する。
 *
 * FREE rangeとallocation sizeが等しい場合は、そのnodeを
 * FREEからALLOCATEDへ遷移させる。
 *
 * FREE rangeの方が大きい場合は、先頭部分をALLOCATED range、
 * 残りを後方FREE rangeへ分割する。
 * 分割に必要なnodeは、create時に事前確保されたnode poolから取得する。
 * 本関数内では動的メモリ確保を行わない。
 *
 * 成功時は対応するALLOCATED nodeのindex、offset、実確保サイズ、および
 * ownerをout_allocation_へ格納する。
 *
 * 本関数はrangeの管理だけを行い、管理対象となる実メモリやGPU bufferへデータを書き込まない。
 *
 * @param[in,out] range_allocator_
 * allocationを行うRange Allocator。
 *
 * @param[in] required_size_
 * 呼び出し側が必要とするrangeサイズ(byte)。
 * 0は指定できない。
 * base alignmentの倍数でない場合は、alignment paddingを加えた実確保サイズへ切り上げられる。
 *
 * @param[in] required_align_
 * allocationに必要なalignment(byte)。
 * 0は指定できず、range_allocator_のbase alignmentと一致しなければならない。
 *
 * @param[out] out_allocation_
 * 確保したrangeのdescriptor格納先。
 * 成功時のみ全fieldが設定される。失敗時は変更されない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * rangeの確保とdescriptorの生成に成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * 次のいずれか。
 * - range_allocator_がNULL
 * - required_size_が0
 * - required_align_が0
 * - out_allocation_がNULL
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * required_align_がrange_allocator_のbase alignmentと一致しない。
 *
 * @retval RANGE_ALLOCATOR_LIMIT_EXCEEDED
 * 現在のallocation countがmax allocation countに到達している。
 *
 * @retval RANGE_ALLOCATOR_NO_MEMORY
 * alignment調整後のallocation sizeを収容できる連続したFREE rangeが存在しない。
 *
 * total free sizeがallocation size以上でも、断片化によって十分な大きさの
 * 連続FREE rangeが存在しない場合は、この結果となる。
 *
 * @retval RANGE_ALLOCATOR_OVERFLOW
 * 次のいずれか。
 * - required_size_へalignment paddingを加えるとsize_tの表現可能範囲を超える
 * - FREE range分割時に後方FREE nodeのoffsetを計算するとsize_tの表現可能範囲を超える
 * - 後方FREE nodeのoffsetとblock sizeからrange終端を計算するとsize_tの表現可能範囲を超える
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - Range Allocatorが内部不変条件を満たしていない
 * - range listがmax node count以内に終端へ到達しない
 * - range list上のnodeがnode poolに所属していない
 * - nodeのstate、range情報、alignment、または接続関係に不整合がある
 * - unused node countと実際のNOT_USED node数が一致しない
 * - FREE range分割の失敗後にrollbackを完遂できなかった
 *
 * @post 成功時は次が成立する。
 * - out_allocation_はlive allocationを表す。
 * - out_allocation_->allocated_sizeはrequired_size_以上であり、
 *   base alignmentの倍数である。
 * - out_allocation_->offsetはbase alignment境界にある。
 * - allocation countが1増加する。
 * - total allocated sizeがout_allocation_->allocated_sizeだけ増加する。
 * - range listはmemory pool全体を引き続き隙間なく表現する。
 *
 * @post 失敗時、out_allocation_は変更されない。
 *
 * @post FREE range分割中の失敗に対するrollbackが成功した場合、
 *       Range Allocatorの内部状態は呼び出し前から変更されない。
 *
 * @warning
 * FREE range分割後のrollback中にnodeのreleaseが失敗した場合は
 * RANGE_ALLOCATOR_DATA_CORRUPTEDを返し、内部状態が呼び出し前と
 * 異なる可能性がある。
 *
 * @par 計算量
 * 現在の実装では、API入口でO(n^2)のdeep validationを実行する。
 * その後のfirst-fit探索、node index検索、および必要に応じた
 * NOT_USED node検索は、それぞれ最大O(n)である。
 *
 * deep validationを除いたallocation処理全体の時間計算量はO(n)である。
 * ここでnはmax node countである。
 *
 * @see range_allocator_free
 * @see range_allocation_t
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
range_allocator_result_t range_allocator_allocate(range_allocator_t* range_allocator_, size_t required_size_, size_t required_align_, range_allocation_t* out_allocation_);

/**
 * @brief allocation descriptorが表すrangeを解放する
 *
 * @details
 * allocation_のowner、node index、offset、およびallocated sizeを検証し、
 * 対応するALLOCATED nodeを特定してFREE状態へ遷移させる。
 *
 * 対象nodeの前後にFREE nodeが存在する場合は、次のいずれかの方法で
 * 隣接rangeをmergeする。
 *
 * - 前後ともFREEではない場合、対象nodeだけをFREEへ遷移させる
 * - 前方だけがFREEの場合、対象rangeを前方FREE rangeへ統合する
 * - 後方だけがFREEの場合、対象rangeを後方FREE rangeと統合する
 * - 前後ともFREEの場合、三つのrangeを一つのFREE rangeへ統合する
 *
 * mergeによって不要になったnodeはrange listから切断し、
 * create時に事前確保されたnode poolへNOT_USED nodeとして返却する。
 *
 * @par Free保証
 * 内部状態が正常であり、range_allocator_allocate()が返した変更されていない
 * live allocation descriptorを、そのownerであるRange Allocatorへ渡した場合、
 * 本関数は成功する。
 *
 * freeでは新しいnodeの取得、動的メモリ確保、および動的メモリ解放を行わない。
 * したがって、validなfreeがnode不足やメモリ不足を理由として失敗することはない。
 *
 * この保証により、range_allocator_allocate()の成功後に上位処理が失敗した場合、
 * 確保したrangeを解放してallocate前の状態へrollbackできる。
 *
 * @param[in,out] range_allocator_
 * allocation_のownerであるRange Allocator。
 * 成功時はrange list、node pool、管理値、およびnodeのstateが更新される。
 *
 * @param[in] allocation_
 * 解放するrangeを表すallocation descriptor。
 *
 * 本関数はdescriptor自体を変更しない。
 * 成功後も各fieldの値は残るが、live allocationを表さなくなるため、
 * 再度本関数へ渡してはならない。
 *
 * @retval RANGE_ALLOCATOR_SUCCESS
 * descriptorに対応するrangeの解放と、必要な隣接FREE rangeのmergeに成功した。
 *
 * @retval RANGE_ALLOCATOR_INVALID_ARGUMENT
 * range_allocator_またはallocation_がNULLである。
 *
 * @retval RANGE_ALLOCATOR_BAD_OPERATION
 * 次のいずれか。
 * - allocation_->allocated_sizeが現在のtotal allocated sizeを超えている
 * - range_allocator_にlive allocationが存在しない
 * - allocation_->allocated_sizeが0
 * - allocation_->ownerがrange_allocator_と一致しない
 * - allocation_->node_indexがnode poolの範囲外
 * - 対応nodeのstateがALLOCATEDではない
 * - 対応nodeのblock sizeがallocation_->allocated_sizeと一致しない
 * - 対応nodeのoffsetがallocation_->offsetと一致しない
 *
 * @retval RANGE_ALLOCATOR_DATA_CORRUPTED
 * 次のいずれか。
 * - Range Allocatorが内部不変条件を満たしていない
 * - range listがmax node count以内に終端へ到達しない
 * - range list上のnodeがnode poolに所属していない
 * - nodeのstate、range情報、alignment、または接続関係に不整合がある
 * - node数、allocation count、unused node count、または
 *   total allocated sizeが実際のnode状態と一致しない
 * - descriptorに対応するnodeまたは隣接FREE nodeが局所的不変条件を満たしていない
 * - descriptorの解決後に、merge対象node、range list、range、
 *   またはnode poolの不整合を検出した
 * - 隣接rangeの終端またはmerge後のblock size計算でoverflowを検出した
 * - 不要になったnodeをrange listから切断またはnode poolへ返却できなかった
 *
 * @post 成功時は次が成立する。
 * - allocation_が表していたrangeはFREE rangeへ統合される
 * - allocation countが1減少する
 * - total allocated sizeがallocation_->allocated_sizeだけ減少する
 * - range listに隣接する二つのFREE nodeは存在しない
 * - range listはmemory pool全体を引き続き隙間なく表現する
 * - allocation_自体の内容は変更されない
 *
 * @post mergeを行わない場合、unused node countは変更されない。
 *
 * @post 前方または後方の一方とmergeした場合、
 *       unused node countが1増加する。
 *
 * @post 前後両方とmergeした場合、unused node countが2増加する。
 *
 * @post nodeおよびrange listの状態変更を開始する前に失敗した場合、
 *       Range Allocatorとallocation_の内容は変更されない。
 *
 * @warning
 * range listからnodeを切断した後に内部データ破損が検出された場合、
 * 呼び出し前の状態へrollbackできない可能性がある。
 *
 * この場合はRANGE_ALLOCATOR_DATA_CORRUPTEDを返すが、
 * allocation countとtotal allocated sizeはcommitされず、
 * Range Allocatorの内部状態が呼び出し前と異なる可能性がある。
 *
 * @warning
 * descriptorはnode generationを保持しない。
 * 解放済みnodeが別のallocationへ再利用され、owner、node index、offset、
 * allocated sizeがすべて一致した場合、stale descriptorを検出できない。
 *
 * @note
 * 本関数はRANGE_ALLOCATOR_NO_MEMORYおよび
 * RANGE_ALLOCATOR_LIMIT_EXCEEDEDを返さない。
 *
 * merge処理中に算術overflowが検出された場合は、
 * validなrangeでは発生しない内部不整合として
 * RANGE_ALLOCATOR_DATA_CORRUPTEDを返す。
 *
 * @par 計算量
 * 現在の実装では、API入口でO(n^2)のdeep validationを実行する。
 *
 * deep validationを除いた場合、mergeを行わないfreeはO(1)である。
 * mergeを行うfreeは、nodeの所属確認のためnode poolを線形探索するためO(n)である。
 * ここでnはmax node countである。
 *
 * 本関数は動的メモリ確保および動的メモリ解放を行わない。
 *
 * @see range_allocator_allocate
 * @see range_allocation_t
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
range_allocator_result_t range_allocator_free(range_allocator_t* range_allocator_, const range_allocation_t* allocation_);

// range_allocator_result_t range_allocator_validate(const range_allocator_t* range_allocator_, range_allocator_validation_result_t* out_validation_result_);

// void range_allocator_validation_result_print(const range_allocator_validation_result_t* validation_result_);

/**
 * @brief Range Allocatorの状態snapshotを取得する
 *
 * @details
 * range_allocator_が保持する容量、alignment、node使用状況、および
 * allocation状況をrange_allocator_status_tへ格納する。
 *
 * 本関数は、内部データが破損している場合でも管理値を観測できるよう、
 * pointerのNULLチェック以外のvalidationを行わない。
 * node poolおよびrange listを走査せず、Range Allocatorが保持する
 * cached管理値だけを使用する。
 *
 * 次の値はRange Allocatorが保持する管理値をそのまま取得する。
 *
 * - memory pool size
 * - base alignment
 * - max node count
 * - max allocation count
 * - total allocated size
 * - unused node count
 * - allocation count
 *
 * 次の値は管理値からO(1)で導出する。
 *
 * @code{.c}
 * total_free_size = memory_pool_size - total_allocated_size;
 * used_node_count = max_node_count - unused_node_count;
 * free_block_count = used_node_count - allocation_count;
 * @endcode
 *
 * 内部データの不整合によっていずれかの減算がunderflowする場合は、
 * 対応する出力値を0へ飽和させる。
 *
 * @param[in] range_allocator_
 * 状態を取得するRange Allocator。
 * NULLの場合は何も行わない。
 *
 * 本関数はRange Allocator、node pool、およびrange listを変更しない。
 *
 * @param[out] out_status_
 * 状態snapshotの格納先。
 *
 * range_allocator_とout_status_がともにNULLでない場合、
 * すべてのfieldが設定される。
 * NULLの場合は何も行わない。
 *
 * @post
 * range_allocator_とout_status_がともにNULLでない場合、
 * out_status_は呼び出し時点におけるRange Allocatorの管理値と、
 * それらから導出した状態値を保持する。
 *
 * @post
 * range_allocator_またはout_status_がNULLの場合、
 * out_status_の内容は変更されない。
 *
 * @post
 * Range Allocator、node pool、およびrange listの状態は変更されない。
 *
 * @note
 * 本関数はrange listを走査しないため、free_block_countは
 * FREE nodeを直接数えた値ではない。
 *
 * 内部状態が正常であれば、管理値から導出したfree_block_countは
 * range list上のFREE node数と一致する。
 *
 * @note
 * total_free_sizeはすべてのFREE rangeの合計サイズであり、
 * 最大の連続FREE rangeサイズではない。
 *
 * したがって、total_free_sizeが要求サイズ以上であっても、
 * 断片化によってallocationに失敗する場合がある。
 *
 * @note
 * 本関数はdeep validationを行わない。
 * 取得したsnapshotだけでは、node pool、range list、または
 * cached管理値の整合性を保証できない。
 *
 * @par 計算量
 * node poolおよびrange listを走査しないため、
 * 時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @see range_allocator_status_t
 * @see range_allocator_status_print
 * @see range_allocator_debug_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void range_allocator_status_get(const range_allocator_t* range_allocator_, range_allocator_status_t* out_status_);

/**
 * @brief Range Allocatorの状態snapshotを標準出力へ表示する
 *
 * @details
 * status_が保持する容量、alignment、node使用状況、および
 * allocation状況をstdoutへ表示する。
 *
 * 次の情報を出力する。
 *
 * - memory usage percent
 * - memory pool size
 * - base alignment
 * - max node count
 * - max allocation count
 * - total allocated size
 * - total free size
 * - unused node count
 * - free block count
 * - used node count
 * - allocation count
 *
 * memory usage percentは次の式で計算する。
 *
 * @code{.c}
 * memory_usage_percent =
 *     total_allocated_size / memory_pool_size * 100.0;
 * @endcode
 *
 * memory pool sizeが0の場合は除算を行わず、
 * memory poolが0であることを示すメッセージを表示する。
 *
 * 出力全体をflockfile()とfunlockfile()で囲み、
 * 同じstdout streamへ協調的に出力する他のthreadとの
 * 行単位の混在を抑止する。
 *
 * 出力には見出しとANSI color sequenceが含まれる。
 *
 * @param[in] status_
 * 表示するRange Allocatorの状態snapshot。
 *
 * NULLの場合もno-opにはせず、status_がNULLであることを示す
 * メッセージをstdoutへ表示する。
 *
 * @post
 * status_の内容は変更されない。
 *
 * @note
 * 本関数はRange Allocatorから状態を取得しない。
 * 現在の状態を表示する場合は、事前にrange_allocator_status_get()を使用して
 * status_へsnapshotを取得する必要がある。
 *
 * @note
 * status_内の各fieldの整合性は検証しない。
 * 内部的に矛盾した値を保持するsnapshotであっても、その内容を表示する。
 *
 * @note
 * fprintf()によるstdoutへの書き込み失敗は呼び出し側へ通知しない。
 *
 * @par 計算量
 * 固定数のfieldを出力するため、時間計算量はO(1)である。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @see range_allocator_status_get
 * @see range_allocator_status_t
 * @see range_allocator_debug_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void range_allocator_status_print(const range_allocator_status_t* status_);

/**
 * @brief Range Allocatorの内部状態を標準出力へdebug表示する
 *
 * @details
 * Range Allocatorの状態snapshot、deep validation結果、および
 * range list上の各nodeをstdoutへ表示する。
 *
 * はじめにrange_allocator_status_get()と同じ状態情報を表示し、
 * 続いてrange_allocator_is_valid()によるdeep validation結果を
 * trueまたはfalseで表示する。
 *
 * range listについて、先頭nodeからaddress orderで走査し、
 * 各nodeの次の情報を表示する。
 *
 * - range list上の走査index
 * - node state
 * - range開始offset
 * - block size
 * - range終端
 *
 * node stateは次のいずれかの文字列として表示する。
 *
 * - ALLOCATED
 * - FREE
 * - TRANSITIONING
 * - NOT_USED
 * - UNDEFINED
 *
 * range終端はoffsetとblock sizeの加算によって求める。
 * 加算がsize_tの表現可能範囲を超える場合は、演算を行わず
 * `end = OVERFLOW`と表示する。
 *
 * @par Range listの走査制限
 * range listの走査node数はmax node countを上限とする。
 *
 * max node count個のnodeを表示した後も次nodeが存在する場合は走査を打ち切り、
 * range listのcycleまたはnode数不整合の可能性を示す
 * `range_node_traversal = TRUNCATED`メッセージを表示する。
 *
 * @par 最大FREE block
 * 走査したFREE nodeのblock sizeからmax free block sizeを求めて表示する。
 *
 * FREE nodeが存在しない場合は0となる。
 * range listの走査が打ち切られた場合は、走査できた範囲だけを対象とした値となる。
 *
 * @param[in] range_allocator_
 * debug表示するRange Allocator。
 *
 * NULLの場合は何も表示せずに終了する。
 * 本関数はRange Allocator、node pool、およびrange listを変更しない。
 *
 * @post
 * range_allocator_がNULLでない場合、状態snapshot、deep validation結果、
 * range list上の各node、およびmax free block sizeがstdoutへ表示される。
 *
 * @post
 * Range Allocator、node pool、およびrange listの状態は変更されない。
 *
 * @note
 * 状態snapshotはcached管理値からO(1)で取得される。
 * 内部データの不整合によって減算がunderflowする場合、
 * 導出値は0へ飽和される。
 *
 * @note
 * stdoutへの出力全体をflockfile()とfunlockfile()で囲み、
 * 同じstreamへ協調的に出力する他のthreadとの行単位の混在を抑止する。
 *
 * stdoutのlockはRange Allocator自体を保護しない。
 * 本関数の実行中に、別のthreadから同じRange Allocatorを
 * allocate、free、またはdestroyしてはならない。
 *
 * @warning
 * 本関数はcycleや過剰なnode数による無制限走査を防止するが、
 * range list内のpointerが参照不可能なaddressを指している状態から
 * 保護するものではない。
 *
 * そのようなpointer破損がある場合、本関数によるnode情報の参照は
 * 未定義動作となる可能性がある。
 *
 * @note
 * fprintf()によるstdoutへの書き込み失敗は呼び出し側へ通知しない。
 * 出力には見出しとANSI color sequenceが含まれる。
 *
 * @par 計算量
 * 現在の実装では、最初にO(n^2)のdeep validationを実行する。
 * その後のrange list走査は最大O(n)であるため、
 * 本関数全体の最悪時間計算量はO(n^2)である。
 * ここでnはmax node countである。
 *
 * 本関数は動的メモリ確保を行わない。
 *
 * @see range_allocator_status_get
 * @see range_allocator_status_print
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
void range_allocator_debug_print(const range_allocator_t* range_allocator_);

#ifdef __cplusplus
}
#endif
#endif
