#ifndef GLCE_TEST_APPLICATION_APPLICATION_CORE_TEST_APPLICATION_ERR_UTILS_H
#define GLCE_TEST_APPLICATION_APPLICATION_CORE_TEST_APPLICATION_ERR_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

// #define TEST_BUILD

#ifdef TEST_BUILD
#include "test_controller.h"

void test_app_rslt_convert_mem_sys_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_linear_alloc_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_platform_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_ring_queue_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_renderer_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_camera_config_set(const test_call_control_t* config_);

void test_app_rslt_convert_texture_system_config_set(const test_call_control_t* config_);

void test_application_err_utils_config_reset(void);

void test_application_err_utils(void);

#endif

#ifdef __cplusplus
}
#endif
#endif
