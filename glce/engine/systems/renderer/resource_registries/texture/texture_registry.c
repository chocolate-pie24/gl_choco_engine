#include "engine/systems/renderer/resource_registries/texture/texture_registry.h"

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h> // for memset
#include <stdbool.h>

#include "engine/base/choco_macros.h"
#include "engine/base/choco_message.h"

#include "engine/core/memory/linear_allocator.h"

#include "engine/containers/choco_string.h"

#include "engine/resource/texture/texture_cpu_resource.h"

#include "engine/systems/renderer/renderer_backend/renderer_backend_context.h"

#include "engine/systems/renderer/resource_registries/core/resource_registry_types.h"
#include "engine/systems/renderer/resource_registries/core/resource_registry_err_utils.h"
#include "engine/systems/renderer/resources/texture/texture_gpu_resource.h"

typedef struct texture_registry_entry {
    choco_string_t* resource_name;
    texture_gpu_resource_t* gpu_resource;
    texture_cpu_resource_t* cpu_resource;
} texture_registry_entry_t;

struct texture_registry {
    size_t max_texture_count;
    texture_registry_entry_t* entries;
};

static void registry_entry_deinitialize(texture_registry_entry_t* registry_entry_, renderer_backend_context_t* backend_context_);

static bool texture_id_is_valid(const texture_registry_t* registry_, int16_t texture_id_);
static bool registry_entry_is_valid(const texture_registry_entry_t* entry_);

static bool find_by_name(const texture_registry_t* registry_, const char* name_, size_t* out_index_);

resource_registry_result_t texture_registry_initialize(size_t max_texture_count_, linear_alloc_t* allocator_, texture_registry_t** out_registry_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
    linear_allocator_result_t ret_linear_alloc = LINEAR_ALLOC_INVALID_ARGUMENT;

    texture_registry_t* tmp_registry = NULL;
    texture_registry_entry_t* tmp_entries = NULL;

    size_t array_size = 0;

    IF_ARG_NULL_GOTO_CLEANUP(allocator_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_initialize", "allocator_")
    IF_ARG_NULL_GOTO_CLEANUP(out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_initialize", "out_registry_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_initialize", "*out_registry_")

    if(0 == max_texture_count_ || INT16_MAX < max_texture_count_) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_initialize(%s) - Provided max_texture_count_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        goto cleanup;
    }

    ret_linear_alloc = linear_allocator_allocate(allocator_, sizeof(texture_registry_t), alignof(texture_registry_t), (void**)&tmp_registry);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("texture_registry_initialize(%s) - Failed to allocate registry instance. target=texture_registry_initialize, bytes=%zu, align=%zu, max_texture_count=%zu", resource_registry_rslt_to_str(ret), sizeof(texture_registry_t), alignof(texture_registry_t), max_texture_count_);
        goto cleanup;
    }
    memset(tmp_registry, 0, sizeof(texture_registry_t));

    if((SIZE_MAX / max_texture_count_) < sizeof(texture_registry_entry_t)) {
        ret = RESOURCE_REGISTRY_OVERFLOW;
        ERROR_MESSAGE("texture_registry_initialize(%s) - array size overflow.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    array_size = sizeof(texture_registry_entry_t) * max_texture_count_;
    ret_linear_alloc = linear_allocator_allocate(allocator_, array_size, alignof(texture_registry_entry_t), (void**)&tmp_entries);
    if(LINEAR_ALLOC_SUCCESS != ret_linear_alloc) {
        ret = resource_registry_rslt_convert_linear_alloc(ret_linear_alloc);
        ERROR_MESSAGE("texture_registry_initialize(%s) - allocation failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    memset(tmp_entries, 0, array_size);

    tmp_registry->max_texture_count = max_texture_count_;
    tmp_registry->entries = tmp_entries;

    *out_registry_ = tmp_registry;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    // リニアアロケータで確保したメモリは個別解放不可であるためクリーンナップ処理はなし
    return ret;
}

// NOTE: このAPIを呼んだ後はmax_texture_countが0になるためregistryは再利用不可となる。再利用を前提で初期化する場合はregistry_reset APIを追加する
void texture_registry_deinitialize(texture_registry_t* registry_, renderer_backend_context_t* backend_context_) {
    if(NULL == registry_ || NULL == backend_context_) {
        ERROR_MESSAGE("texture_registry_deinitialize(%s) - provided registry_ or backend_context_ is NULL.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return;
    }
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_deinitialize(%s) - texture_registry_t internal state is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return;
    }
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        registry_entry_deinitialize(&registry_->entries[i], backend_context_);
    }
    registry_->max_texture_count = 0;
}

bool texture_registry_find(const texture_registry_t* registry_, const char* name_) {
    size_t tmp_id = 0;

    if(NULL == registry_ || NULL == name_) {
        return false;
    }
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_find(%s) - texture_registry_t internal state is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return false;
    }
    if('\0' == name_[0]) {
        ERROR_MESSAGE("texture_registry_find(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return false;
    }

    return find_by_name(registry_, name_, &tmp_id);
}

const char* texture_registry_name_get(const texture_registry_t* registry_, int16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided registry_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
    if(!texture_id_is_valid(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_name_get(%s) - provided texture_id_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }
    if(NULL == registry_->entries[texture_id_].resource_name) {
        return NULL;
    }
    return choco_string_c_str(registry_->entries[texture_id_].resource_name);
}

const texture_gpu_resource_t* texture_registry_gpu_resource_get(const texture_registry_t* registry_, int16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided registry_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
    if(!texture_id_is_valid(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_gpu_resource_get(%s) - provided texture_id_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].gpu_resource;
}

const texture_cpu_resource_t* texture_registry_cpu_resource_get(const texture_registry_t* registry_, int16_t texture_id_) {
    if(NULL == registry_) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided registry_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT));
        return NULL;
    }
    if(!texture_registry_is_valid(registry_)) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED));
        return NULL;
    }
    if(!texture_id_is_valid(registry_, texture_id_)) {
        ERROR_MESSAGE("texture_registry_cpu_resource_get(%s) - provided texture_id_ is not valid.", resource_registry_rslt_to_str(RESOURCE_REGISTRY_BAD_OPERATION));
        return NULL;
    }

    return registry_->entries[texture_id_].cpu_resource;
}

resource_registry_result_t texture_registry_id_get(const texture_registry_t* registry_, const char* name_, int16_t* out_texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    size_t tmp_id = 0;

    IF_ARG_NULL_GOTO_CLEANUP(name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "name_")
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_id_get", "out_texture_id_")

    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_id_get(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if('\0' == name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_id_get(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    if(!find_by_name(registry_, name_, &tmp_id)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_id_get(%s) - find_by_name failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    *out_texture_id_ = (int16_t)tmp_id;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

// cpu_resource, gpu_resourceをregistryにmoveする
// TODO: geometry側もmoveする仕様に変更する
resource_registry_result_t texture_registry_register(texture_registry_t* registry_, const char* resource_name_, texture_gpu_resource_t** gpu_resource_, texture_cpu_resource_t** cpu_resource_, int16_t* out_texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    choco_string_result_t ret_string = CHOCO_STRING_INVALID_ARGUMENT;

    size_t tmp_index = 0;
    bool found_free_slot = false;
    choco_string_t* tmp_name = NULL;

    // 入力値検証
    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(resource_name_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "resource_name_")
    IF_ARG_NULL_GOTO_CLEANUP(gpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(*gpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "*gpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(cpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "cpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(*cpu_resource_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "*cpu_resource_")
    IF_ARG_NULL_GOTO_CLEANUP(out_texture_id_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_register", "out_texture_id_")
    if('\0' == resource_name_[0]) {
        ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;
        ERROR_MESSAGE("texture_registry_register(%s) - provided resource name is not valid.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_registry_is_valid(registry_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_register(%s) - provided registry_ is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_cpu_resource_is_valid(*cpu_resource_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_register(%s) - provided CPU resource is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_cpu_resource_is_loaded(*cpu_resource_)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_register(%s) - provided CPU resource is not loaded.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_gpu_resource_is_valid(*gpu_resource_)) {
        ret = RESOURCE_REGISTRY_DATA_CORRUPTED;
        ERROR_MESSAGE("texture_registry_register(%s) - provided GPU resource is corrupted.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }
    if(!texture_gpu_resource_is_uploaded(*gpu_resource_)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_register(%s) - provided GPU resource is not uploaded.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // リソースの重複チェック
    if(find_by_name(registry_, resource_name_, &tmp_index)) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_register(%s) - provided resource name is already registered.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // 空きスロット検索
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(NULL == registry_->entries[i].resource_name) {
            found_free_slot = true;
            tmp_index = i;
            break;
        }
    }
    if(!found_free_slot) {
        ret = RESOURCE_REGISTRY_LIMIT_EXCEEDED;
        ERROR_MESSAGE("texture_registry_register(%s) - free slot not found.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // リソース名称生成
    ret_string = choco_string_create_from_c_string(resource_name_, &tmp_name);
    if(CHOCO_STRING_SUCCESS != ret_string) {
        ret = resource_registry_rslt_convert_choco_string(ret_string);
        ERROR_MESSAGE("texture_registry_register(%s) - choco_string_create_from_c_string failed.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    // entry登録, 所有権移動commit
    registry_->entries[tmp_index].cpu_resource = *cpu_resource_;
    registry_->entries[tmp_index].gpu_resource = *gpu_resource_;
    registry_->entries[tmp_index].resource_name = tmp_name;

    *cpu_resource_ = NULL;
    *gpu_resource_ = NULL;
    tmp_name = NULL;

    *out_texture_id_ = (int16_t)tmp_index;

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    if(RESOURCE_REGISTRY_SUCCESS != ret) {
        if(NULL != tmp_name) {
            choco_string_destroy(&tmp_name);
        }
    }
    return ret;
}

resource_registry_result_t texture_registry_unregister(texture_registry_t* registry_, renderer_backend_context_t* backend_context_, int16_t texture_id_) {
    resource_registry_result_t ret = RESOURCE_REGISTRY_INVALID_ARGUMENT;

    IF_ARG_NULL_GOTO_CLEANUP(registry_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_unregister", "registry_")
    IF_ARG_NULL_GOTO_CLEANUP(backend_context_, ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_unregister", "backend_context_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_registry_is_valid(registry_), ret, RESOURCE_REGISTRY_DATA_CORRUPTED, resource_registry_rslt_to_str(RESOURCE_REGISTRY_DATA_CORRUPTED), "texture_registry_unregister", "registry_")
    IF_ARG_FALSE_GOTO_CLEANUP(texture_id_is_valid(registry_, texture_id_), ret, RESOURCE_REGISTRY_INVALID_ARGUMENT, resource_registry_rslt_to_str(RESOURCE_REGISTRY_INVALID_ARGUMENT), "texture_registry_unregister", "texture_id_")

    if(NULL == registry_->entries[texture_id_].resource_name) {
        ret = RESOURCE_REGISTRY_BAD_OPERATION;
        ERROR_MESSAGE("texture_registry_unregister(%s) - provided texture id entry is empty.", resource_registry_rslt_to_str(ret));
        goto cleanup;
    }

    registry_entry_deinitialize(&registry_->entries[texture_id_], backend_context_);

    ret = RESOURCE_REGISTRY_SUCCESS;

cleanup:
    return ret;
}

bool texture_registry_is_valid(const texture_registry_t* registry_) {
    if(NULL == registry_) {
        return false;
    }
    if(0 == registry_->max_texture_count || INT16_MAX < registry_->max_texture_count) {
        return false;
    }
    if(NULL == registry_->entries) {
        return false;
    }
    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(!registry_entry_is_valid(&registry_->entries[i])) {
            return false;
        }
    }
    return true;
}

// NOTE: registry_entry_のvalidationを上位側で実行しておくこと
static void registry_entry_deinitialize(texture_registry_entry_t* registry_entry_, renderer_backend_context_t* backend_context_) {
    if(NULL == registry_entry_ || NULL == backend_context_) {
        return;
    }
    choco_string_destroy(&registry_entry_->resource_name);
    texture_cpu_resource_destroy(&registry_entry_->cpu_resource);
    texture_gpu_resource_destroy(backend_context_, &registry_entry_->gpu_resource);
}

static bool texture_id_is_valid(const texture_registry_t* registry_, int16_t texture_id_) {
    if(NULL == registry_) {
        return false;
    }
    if(texture_id_ < 0 || registry_->max_texture_count <= (size_t)texture_id_) {
        return false;
    }
    return true;
}

static bool registry_entry_is_valid(const texture_registry_entry_t* entry_) {
    if(NULL == entry_) {
        return false;
    }

    // 初期化直後の未使用entryは正常
    if(NULL == entry_->cpu_resource && NULL == entry_->gpu_resource && NULL == entry_->resource_name) {
        return true;
    }

    // 中途半端な初期化状態は異常(全NULL判定済みなので、いずれかがNULLの場合は部分初期化状態)
    if(NULL == entry_->cpu_resource || NULL == entry_->gpu_resource || NULL == entry_->resource_name) {
        return false;
    }

    // 登録済みentryの検査
    if(0 == choco_string_length(entry_->resource_name)) {
        return false;
    }
    if(!texture_cpu_resource_is_valid(entry_->cpu_resource)) {
        return false;
    }
    if(!texture_cpu_resource_is_loaded(entry_->cpu_resource)) { // 登録済みで未ロード状態は異常
        return false;
    }
    if(!texture_gpu_resource_is_valid(entry_->gpu_resource)) {
        return false;
    }
    if(!texture_gpu_resource_is_uploaded(entry_->gpu_resource)) {   // 登録済みで未アップロード状態は異常
        return false;
    }
    return true;
}

static bool find_by_name(const texture_registry_t* registry_, const char* name_, size_t* out_index_) {
    size_t tmp_slot = 0;
    bool found = false;

    if(NULL == name_ || NULL == registry_ || NULL == out_index_) {
        return false;
    }

    for(size_t i = 0; i != registry_->max_texture_count; ++i) {
        if(NULL != registry_->entries[i].resource_name && choco_string_equal(choco_string_c_str(registry_->entries[i].resource_name), name_)) {
            tmp_slot = i;
            found = true;
            break;
        }
    }
    if(found) {
        *out_index_ = tmp_slot;
    }
    return found;
}
