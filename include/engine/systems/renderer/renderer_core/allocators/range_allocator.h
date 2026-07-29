/**
 * @file range_allocator.h
 * @brief GPU buffer内の固定alignment rangeを管理するRange Allocatorの公開API
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
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_ALLOCATORS_RANGE_ALLOCATOR_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RENDERER_CORE_ALLOCATORS_RANGE_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Range Allocatorの内部状態を保持するopaque型
 *
 * @details
 * 呼び出し側は内部メンバーへ直接アクセスせず、
 * Range Allocatorの公開APIを通して本型を操作する。
 *
 * インスタンスはrange_allocator_create()によって生成され、
 * range_allocator_destroy()によって破棄される。
 *
 * @see range_allocator_create
 * @see range_allocator_destroy
 */
typedef struct range_allocator range_allocator_t;
// typedef struct range_allocator_validation_result range_allocator_validation_result_t;   /**< Range Allocator validation結果構造体のopaque型 */

/**
 * @brief Range Allocatorの実行結果コード
 *
 * @details
 * Range Allocatorの処理結果を表す。
 *
 * 引数そのものの不備、API契約に反する操作、管理上限への到達、
 * rangeまたはメモリの不足、内部データ破損、および算術overflowを区別して呼び出し側へ通知する。
 *
 * 各結果コードの具体的な発生条件と、失敗時の内部状態および出力引数に関する保証は、それぞれのAPIのDoxygenに記載する。
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef enum {
    RANGE_ALLOCATOR_SUCCESS = 0,        /**< 処理に成功した */
    RANGE_ALLOCATOR_INVALID_ARGUMENT,   /**< NULL pointer、0を許可しないsizeやcount、または出力先の初期状態など、引数の基本的な事前条件を満たしていない */
    RANGE_ALLOCATOR_LIMIT_EXCEEDED,     /**< max allocation count、node pool、または下位メモリ管理機構の管理上限に到達した */
    RANGE_ALLOCATOR_NO_MEMORY,          /**< Range Allocator生成に必要な動的メモリを確保できなかった、または要求サイズを収容できる連続したFREE rangeが存在しない
                                         *
                                         *   total free sizeが要求サイズ以上でも、断片化によって十分な大きさの連続FREE rangeが存在しない場合は、この結果となる。 */
    RANGE_ALLOCATOR_DATA_CORRUPTED,     /**< node pool、range list、管理値などの内部状態が不変条件を満たしていない、
                                         *   または正常な内部状態では成功するはずの処理が失敗した */
    RANGE_ALLOCATOR_BAD_OPERATION,      /**< 引数の基本形式は有効だが、現在の状態またはRange AllocatorのAPI契約と両立しない操作が要求された
                                         *
                                         *   base alignmentとの不一致、現在の状態では許可されないnode操作、またはlive allocationを示さないdescriptorの使用などが該当する。
                                         */
    RANGE_ALLOCATOR_OVERFLOW,           /**< node数、node poolサイズ、alignment調整後の実確保サイズ、またはrange終端などのsize_t演算が表現可能範囲を超える */
    RANGE_ALLOCATOR_UNDEFINED_ERROR,    /**< 下位モジュールから未定義または変換対象外の結果コードを受け取った */
} range_allocator_result_t;

/**
 * @brief Range Allocatorから取得したallocationを表すdescriptor
 *
 * @details
 * range_allocator_allocate()によって確保されたrangeの位置と実確保サイズ、
 * 対応するALLOCATED nodeのidentity、および生成元Range Allocatorを保持する。
 *
 * @par Range情報
 * offsetは、Range Allocatorが管理する論理メモリプールの先頭から
 * allocation開始位置までのbyte offsetを表す。
 *
 * allocated_sizeはrequired sizeをbase alignment単位に切り上げた実確保サイズであり、
 * 呼び出し側が要求したデータサイズと一致しない場合がある。
 * Range Allocatorが占有およびfreeする範囲には、このalignment paddingも含まれる。
 *
 * @par Allocation identity
 * ownerとnode_indexの組み合わせによって、allocationに対応する
 * ALLOCATED nodeを特定する。
 *
 * free時には、owner、node_index、offset、allocated_size、および
 * 対応nodeのstateが検証される。
 * privateなnodeへのpointerは公開しない。
 *
 * @par Descriptorの生存期間
 * 本descriptorは、ownerが生存し、対応するallocationがfreeされるまで有効である。
 *
 * range_allocator_free()はdescriptor自体を変更しない。
 * free成功後も各fieldの値は残るが、descriptorはlive allocationを表さなくなるため、
 * 再度freeへ渡してはならない。
 *
 * descriptorを複製してもallocationは複製されない。
 * 複数のdescriptorから同じallocationをfreeするとdouble freeになる。
 *
 * ownerをrange_allocator_destroy()によって破棄した場合、そのownerを保持する
 * すべてのdescriptorは無効となる。
 *
 * @par Stale descriptor検出の制限
 * nodeのgenerationは保持しない。
 * 解放済みnodeが別のallocationに再利用され、owner、node_index、offset、
 * allocated_sizeがすべて一致した場合、古いstale descriptorを検出できない。
 *
 * @see range_allocator_allocate
 * @see range_allocator_free
 *
 * @par AI支援
 * このドキュメントはChatGPT Work（OpenAI Codex）を用いて草案を生成し、
 * プロジェクト作成者が内容を確認・修正した。
 */
typedef struct range_allocation {
    size_t offset;                  /**< 論理メモリプール先頭からのallocation開始offset(byte) */
    size_t allocated_size;          /**< base alignment調整後にRange Allocatorが占有する実確保サイズ(byte) */
    size_t node_index;              /**< 対応するALLOCATED nodeのnode pool index */
    const range_allocator_t* owner; /**< このallocationを生成したRange Allocator */
} range_allocation_t;

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
 * - FREE range分割時の後方FREE nodeのoffset計算がsize_tの表現可能範囲を超える
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

range_allocator_result_t range_allocator_free(range_allocator_t* range_allocator_, const range_allocation_t* allocation_);

// range_allocator_result_t range_allocator_validate(const range_allocator_t* range_allocator_, range_allocator_validation_result_t* out_validation_result_);

// void range_allocator_validation_result_print(const range_allocator_validation_result_t* validation_result_);

void range_allocator_status_get(const range_allocator_t* range_allocator_, range_allocator_status_t* out_status_);

void range_allocator_status_print(const range_allocator_status_t* status_);

void range_allocator_debug_print(const range_allocator_t* range_allocator_);

#ifdef __cplusplus
}
#endif
#endif
