// SPDX-License-Identifier: MIT
// Copyright (c) 2026 chocolate-pie24

/** @ingroup core
 *
 * @file buffer_utils.c
 * @author chocolate-pie24
 * @brief データが密にパックされたバッファへのデータの書き込みと、バッファからのデータの取り出しAPIの実装
 *
 * @date 2026-05-14
 *
 */
#include <stdint.h>
#include <stddef.h>

#include "engine/core/buffer_utils/buffer_utils.h"

/**
 * @brief メモリアライメントを考慮しデータを取り出すための一時バッファ共用体
 *
 */
typedef union buffer_utils {
    char buff_char[8];          /**< char型バッファ */
    int16_t buff_int16_t[4];    /**< int16_t型バッファ */
    int32_t buff_int32_t[2];    /**< int32_t型バッファ */
    int64_t buff_int64_t;       /**< int64_t型バッファ */
    uint16_t buff_uint16_t[4];  /**< uint16_t型バッファ */
    uint32_t buff_uint32_t[2];  /**< uint32_t型バッファ */
    uint64_t buff_uint64_t;     /**< uint64_t型バッファ */
} buff_utils_t;

int16_t buffer_utils_le_int16_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    return convert.buff_int16_t[0];
}

int32_t buffer_utils_le_int32_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    convert.buff_char[2] = buffer_[2];
    convert.buff_char[3] = buffer_[3];
    return convert.buff_int32_t[0];
}

int64_t buffer_utils_le_int64_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    convert.buff_char[2] = buffer_[2];
    convert.buff_char[3] = buffer_[3];
    convert.buff_char[4] = buffer_[4];
    convert.buff_char[5] = buffer_[5];
    convert.buff_char[6] = buffer_[6];
    convert.buff_char[7] = buffer_[7];
    return convert.buff_int64_t;
}

uint16_t buffer_utils_le_uint16_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    return convert.buff_uint16_t[0];
}

uint32_t buffer_utils_le_uint32_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    convert.buff_char[2] = buffer_[2];
    convert.buff_char[3] = buffer_[3];
    return convert.buff_uint32_t[0];
}

uint64_t buffer_utils_le_uint64_t_get(const char* buffer_) {
    buff_utils_t convert;
    convert.buff_char[0] = buffer_[0];
    convert.buff_char[1] = buffer_[1];
    convert.buff_char[2] = buffer_[2];
    convert.buff_char[3] = buffer_[3];
    convert.buff_char[4] = buffer_[4];
    convert.buff_char[5] = buffer_[5];
    convert.buff_char[6] = buffer_[6];
    convert.buff_char[7] = buffer_[7];
    return convert.buff_uint64_t;
}
