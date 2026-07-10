// NOTE: 本モジュールは、将来的にrenderer/renderer/core/allocatorsに移動し、当面はOpenGLバックエンド用のVBO / EBO管理専用のアロケータとする。
// 以下の理由により、CPU側メモリリソース確保には使用しない
//
// 理由1:
// freeするためにallocated_sizeが必要、allocated_sizeはアライメントの都合上、required_sizeとは異なる場合がある。
// CPU側では通常、sizeof(type) x countでメモリ使用量を管理することが多く、個別に確保サイズ変数を記憶しておくことは少ない。
// VBOではoffsetを管理する必要があるため、この問題は影響が少ないが、CPU側では使いにくく、APIの誤用が増える可能性がある。
//
// 理由2:
// Range Free Listでは、空きノード数に上限を設けている。このため、メモリチャンクが多く発生すると、空きノードが足りなくなり、freeに失敗する可能性がある
// CPU側リソースでは、アセットのロード中など、頻繁なアロケーションと頻繁なfreeが発生する場合がある。
// このため、起動時に正確な空きノードの数を見積もるのが難しい。よって、CPU側メモリリソース確保には、Range Free Listではなく、通常のFree Listを使用することにする。
// GPU側リソース確保についても、同様の問題は発生するが、CPU側のように通常のFree ListはメモリプールがGPU上にある都合上使用できない。そのためRange Free Listを使用するが、
// 以下の対策を行うことで、空きノード不足に対応する
// - vbo_poolを導入し、dynamic_arrayで管理する
// - vbo_poolはvbo_pageの配列で、vbo_pageはvboとrange_free_listを保持する
// - 空きノードが足りなくなった場合、vbo_poolの配列要素を増やす
#ifndef GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H
#define GLCE_ENGINE_CORE_MEMORY_RANGE_FREE_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct range_free_list range_free_list_t;   /**< Range Free List内部状態管理構造体のopaque型 */

typedef enum {
    RANGE_FREE_LIST_SUCCESS = 0,
    RANGE_FREE_LIST_INVALID_ARGUMENT,
    RANGE_FREE_LIST_LIMIT_EXCEEDED,
    RANGE_FREE_LIST_NO_MEMORY,
    RANGE_FREE_LIST_DATA_CORRUPTED,
    RANGE_FREE_LIST_BAD_OPERATION,
    RANGE_FREE_LIST_OVERFLOW,
    RANGE_FREE_LIST_UNDEFINED_ERROR,
} range_free_list_result_t;

typedef struct range_allocation {
    size_t offset;
    size_t allocated_size;
} range_allocation_t;

typedef struct range_free_list_status {
    size_t memory_pool_size;
    size_t base_align;

    size_t max_node_count;
    size_t unused_node_count;
    size_t free_block_count;

    size_t total_free_size;
    size_t max_free_block_size;
} range_free_list_status_t;

range_free_list_result_t range_free_list_create(size_t memory_pool_size_, size_t max_node_count_, size_t base_align_, range_free_list_t** out_range_free_list_);

void range_free_list_destroy(range_free_list_t** range_free_list_);

range_free_list_result_t range_free_list_allocate(range_free_list_t* range_free_list_, size_t required_size_, size_t required_align_, range_allocation_t* out_allocation_);

range_free_list_result_t range_free_list_free(range_free_list_t* range_free_list_, range_allocation_t allocation_);

void range_free_list_status_get(const range_free_list_t* range_free_list_, range_free_list_status_t* out_status_);

void range_free_list_debug_print(const range_free_list_t* range_free_list_);

#ifdef __cplusplus
}
#endif
#endif
