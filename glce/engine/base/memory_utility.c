#include "engine/base/memory_utility.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

static bool is_power_of_two(size_t value_);  // choco_macrosのIS_POWER_OF_TWOと使用しても良いが、choco_macrosへの依存を避けるため用意した

bool memory_utility_align_up(size_t value_, size_t alignment_, size_t* out_aligned_size_) {
    if(NULL == out_aligned_size_) {
        return false;
    }
    if(!is_power_of_two(alignment_)) {
        return false;
    }

    // alignment_未満の端数を切り捨てられるよう、切り上げ分を加算する
    if((SIZE_MAX - value_) < (alignment_ - 1)) {
        return false;
    }
    const size_t padded_size = value_ + (alignment_ - 1);

    // alignment_未満の端数を切り捨て、alignment_の倍数へ揃える
    *out_aligned_size_ = padded_size & ~(alignment_ - 1);

    return true;
}

bool memory_utility_is_aligned(size_t value_, size_t alignment_, bool* out_is_aligned_) {
    if(NULL == out_is_aligned_) {
        return false;
    }
    if(!is_power_of_two(alignment_)) {
        return false;
    }
    *out_is_aligned_ = ((value_) & ((alignment_) - 1)) == 0;

    return true;
}

bool memory_utility_padding_calc(size_t value_, size_t alignment_, size_t* out_padding_size_) {
    size_t aligned_size = 0;

    if(NULL == out_padding_size_) {
        return false;
    }
    if(!is_power_of_two(alignment_)) {
        return false;
    }
    if(!memory_utility_align_up(value_, alignment_, &aligned_size)) {
        return false;
    }

    *out_padding_size_ = aligned_size - value_;

    return true;
}

static bool is_power_of_two(size_t value_) {
    if((value_ == 0u) || (value_ & (value_ - 1)) != 0u) {
        return false;
    }
    return true;
}
