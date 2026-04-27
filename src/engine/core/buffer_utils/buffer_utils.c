#include <stdint.h>
#include <stddef.h>

#include "engine/core/buffer_utils/buffer_utils.h"

#ifdef TEST_BUILD
#include <assert.h>
#include <stdbool.h>
#endif

typedef union buffer_utils {
    char buff_char[8];
    int16_t buff_int16_t[4];
    int32_t buff_int32_t[2];
    int64_t buff_int64_t;
    uint16_t buff_uint16_t[4];
    uint32_t buff_uint32_t[2];
    uint64_t buff_uint64_t;
} buff_utils_t;

int16_t buffer_utils_le_int16_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    return convert.buff_int16_t[0];
}

int32_t buffer_utils_le_int32_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    convert.buff_char[2] = buff_[2];
    convert.buff_char[3] = buff_[3];
    return convert.buff_int32_t[0];
}

int64_t buffer_utils_le_int64_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    convert.buff_char[2] = buff_[2];
    convert.buff_char[3] = buff_[3];
    convert.buff_char[4] = buff_[4];
    convert.buff_char[5] = buff_[5];
    convert.buff_char[6] = buff_[6];
    convert.buff_char[7] = buff_[7];
    return convert.buff_int64_t;
}

uint16_t buffer_utils_le_uint16_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    return convert.buff_uint16_t[0];
}

uint32_t buffer_utils_le_uint32_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    convert.buff_char[2] = buff_[2];
    convert.buff_char[3] = buff_[3];
    return convert.buff_uint32_t[0];
}

uint64_t buffer_utils_le_uint64_t_get(const char* buff_) {
#ifdef TEST_BUILD
    if(NULL == buff_) {
        assert(false);
    }
#endif
    buff_utils_t convert;
    convert.buff_char[0] = buff_[0];
    convert.buff_char[1] = buff_[1];
    convert.buff_char[2] = buff_[2];
    convert.buff_char[3] = buff_[3];
    convert.buff_char[4] = buff_[4];
    convert.buff_char[5] = buff_[5];
    convert.buff_char[6] = buff_[6];
    convert.buff_char[7] = buff_[7];
    return convert.buff_uint64_t;
}
