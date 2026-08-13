#ifndef GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_ALLOCATORS_CORE_RANGE_ALLOCATOR_TYPES_H
#define GLCE_ENGINE_SYSTEMS_RENDERER_RESOURCES_ALLOCATORS_CORE_RANGE_ALLOCATOR_TYPES_H

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

#ifdef __cplusplus
}
#endif
#endif
