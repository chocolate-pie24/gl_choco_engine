#ifndef GLCE_ENGINE_PLATFORM_CONCRETES_INTERNAL_PLATFORM_GLFW_INTERNAL_H
#define GLCE_ENGINE_PLATFORM_CONCRETES_INTERNAL_PLATFORM_GLFW_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "engine/core/platform/platform_utils.h"

typedef struct test_control_return_code {
    bool test_enable;
    platform_result_t result;   /**< 強制的にこのエラーコードを返すようにする */
} test_control_return_code_t;

typedef struct test_control_glfw_init {
    bool test_enable;
    int result;
} test_control_glfw_init_t;

typedef struct test_control_glfw {
    test_control_glfw_init_t glfw_init;
    test_control_return_code_t result_control;
} test_control_glfw_t;

#ifdef __cplusplus
}
#endif
#endif
