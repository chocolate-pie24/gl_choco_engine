# GLCE Unit Test Code Implementation Guide

This document describes how to implement unit tests for a module’s public APIs and private functions.

## Failure Injection Mechanism for Testing

During testing, it is necessary to have a mechanism that forces the function under test to return a specified value.

In GLCE, this mechanism is provided only in test builds so that all modules can use failure injection in a consistent manner.

This mechanism is defined in test/include/test_controller.h and implemented in test/src/test_controller.c.

test_controller.h exposes the following to external modules. Failure injection control structures are provided according to the return type of the target function.

However, if the return type is module-specific and cannot be represented by the common failure injection control structures, defining such a structure would introduce a dependency on that module. In that case, this failure injection mechanism must not be used, and the module must handle it individually.

```c
/**
 * @brief Failure injection control structure for APIs whose return type is a result code or int
 *
 */
typedef struct test_call_control {
    uint32_t call_count;    /**< Number of API calls */
    uint32_t fail_on_call;  /**< Specifies on which call the API should fail (0 = disabled, normal execution; 1 or greater = return forced_result) */
    int forced_result;      /**< Return value to force when injecting failure (use by casting each result code to int) */
} test_call_control_t;

/**
 * @brief Failure injection control structure for APIs whose return type is size_t
 *
 */
typedef struct test_call_control_size_t {
    uint32_t call_count;    /**< Number of API calls */
    uint32_t fail_on_call;  /**< Specifies on which call the API should fail (0 = disabled, normal execution; 1 or greater = return forced_result) */
    size_t forced_result;   /**< Return value to force when injecting failure */
} test_call_control_size_t_t;

/**
 * @brief Failure injection control structure for APIs whose return type is bool
 *
 */
typedef struct test_call_control_bool {
    uint32_t call_count;    /**< Number of API calls */
    uint32_t fail_on_call;  /**< Specifies on which call the API should fail (0 = disabled, normal execution; 1 or greater = return forced_result) */
    bool forced_result;     /**< Return value to force when injecting failure */
} test_call_control_bool_t;

/**
 * @brief Resets the fields of the failure injection control structure so that failure injection is disabled
 *
 * @details After reset, the structure fields have the following values:
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0 (in many cases SUCCESS)
 *
 * @param[out] control_ Pointer to the control structure to reset
 */
void test_call_control_reset(test_call_control_t* control_);

/**
 * @brief Resets the fields of the failure injection control structure so that failure injection is disabled
 *
 * @details After reset, the structure fields have the following values:
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0
 *
 * @param[out] control_ Pointer to the control structure to reset
 */
void test_call_control_size_t_reset(test_call_control_size_t_t* control_);

/**
 * @brief Resets the fields of the failure injection control structure so that failure injection is disabled
 *
 * @details After reset, the structure fields have the following values:
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = false
 *
 * @param[out] control_ Pointer to the control structure to reset
 */
void test_call_control_bool_reset(test_call_control_bool_t* control_);
```

## Procedure for Creating Unit Test Code

### Creating a Test Header File

Create the test header file for the target module according to the following rules.

- The directory for the header file must mirror the hierarchy of the target module.
- The test header file name must be test_module_name.h.

For example, if the target is include/engine/core/memory/choco_memory.h, then the test header file should be test/include/engine/core/memory/test_choco_memory.h.

The test header file must contain the following.

#### Failure Injection APIs

For each public API in the module that can use failure injection with the types provided by test_controller.h, define a dedicated failure injection API.

The naming convention for a failure injection API is test_foo_config_set, where foo represents the public API name.

This API is used as follows.
Note that the call_count field is managed inside the module, so the side injecting failure must not set it.

```c
test_call_control_t config = { 0 };
test_call_control_reset(&config);

config.fail_on_call = 1;                // Fail on the first call
config.forced_result = (int)xxx_result; // Force API foo to return xxx module's result code

test_foo_config_set(&config);           // Inject failure into API foo
```

test_foo_config_set is implemented as follows.

```c
void test_foo_config_set(const test_call_control_t* config_) {
    if(NULL == config_) {
        assert(false);
        return;
    }
    s_test_config_foo.fail_on_call = config_->fail_on_call;
    s_test_config_foo.forced_result = config_->forced_result;
}
```

#### Module Test API

Define an API that tests all public APIs and private functions owned by the module.

The naming convention is test_xxx, where xxx represents the module name.

#### Resetting Failure Injection Settings

Define an API that resets all failure injection settings owned by the module.

The naming convention is test_module_name_config_reset.

Note that test_module_name_config_reset resets not only failure injection settings, but also all other test-related settings such as mock function test settings and test scenario settings.

A test header containing the above should look like this.

```c
#ifndef GLCE_TEST_PATH_MODULE_NAME_H
#define GLCE_TEST_PATH_MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"    // Include the test infrastructure header

// Failure injection APIs for public APIs
// (provide one for each public API that can be handled by the failure injection types defined in test_controller.h)
void test_foo_config_set(const test_call_control_t* config_);   // Failure injection setting API for API foo
void test_bar_config_set(const test_call_control_t* config_);   // Failure injection setting API for API bar

// Reset all test-related settings inside the module
// (disables all test-only behavior, including failure injection as well as mock function test settings and test scenario settings)
void test_module_name_config_reset(void);

// Test routine for all functions owned by the module (public APIs and private functions)
void test_module_name(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
```

### Adding Test Implementations

All test-related processing is written in the implementation file of the module under test.

This allows direct access to fields of the various structures used by the module, keeping the test code simple.

#### Test Data Structure Definitions and Function Prototype Section

Below the include directives in the module implementation file, prepare a TEST_BUILD code block and write the following.

To keep all test functions consistent, the order must be unified across all modules.

- Include directives for headers used only during testing
- Declarations of failure injection control structure instances for public APIs that use failure injection(instance name: s_test_config_xxx, where xxx is the public API name)
- If needed, declarations of failure injection control structure instances for private functions that use failure injection(instance name: s_test_config_xxx, where xxx is the private function name)
- Prototypes for all test functions(test function name: test_xxx, where xxx represents the target function name)
- The order of the prototypes must match the implementation order of the test functions

Each test function is added after the implementation of the module’s public APIs and private functions.

The implementation order of test functions is as follows.

- The test function that executes all test functions(the function declared in the test header)
- Individual unit test functions for each function(implemented in the same order as the prototypes)

```c
#ifdef TEST_BUILD
// Include directives used only during testing
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memset strcmp
#include "xxx/test_xxx.h"   // Test header for module xxx

// Public API test settings
static test_call_control_t s_test_config_foo;    /**< Test setting for API foo */
static test_call_control_t s_test_config_bar;    /**< Test setting for API bar */

// Private function test settings (add declaration if needed)
static test_call_control_t s_test_config_baz;    /**< Test setting for private function baz */

// Prototypes for all test functions
// (these are called from the module test function; the module test function is named test_module_name)
static void test_xxx(void); /**< Test function for each target function; xxx is replaced with the function name */
#endif
```

### Failure Injection Processing in Public API Implementations

To make return values controllable from upper-layer calls, all public APIs that are handled by the common failure injection mechanism or a module-specific failure injection mechanism must provide functionality to force a specified return value.

Because forced return behavior must take effect regardless of normal error handling, it must be placed at the beginning of the function.

For example:

```c
memory_system_result_t memory_system_allocate(size_t size_, memory_tag_t mem_tag_, void** out_ptr_) {
#ifdef TEST_BUILD
    s_test_config_memory_system_allocate.call_count++;
    if(s_test_config_memory_system_allocate.fail_on_call != 0) {
        if(s_test_config_memory_system_allocate.call_count == s_test_config_memory_system_allocate.fail_on_call) {
            return (memory_system_result_t)s_test_config_memory_system_allocate.forced_result;
        }
    }
#endif
    memory_system_result_t ret = MEMORY_SYSTEM_INVALID_ARGUMENT;
    void* tmp = NULL;

    // Preconditions.
    IF_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, ret, MEMORY_SYSTEM_BAD_OPERATION, rslt_to_str(MEMORY_SYSTEM_BAD_OPERATION), "memory_system_allocate", "s_mem_sys_ptr")
    IF_ARG_NULL_GOTO_CLEANUP(out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "out_ptr_")
    IF_ARG_NOT_NULL_GOTO_CLEANUP(*out_ptr_, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "*out_ptr_")
    IF_ARG_FALSE_GOTO_CLEANUP(mem_tag_ < MEMORY_TAG_MAX, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "mem_tag_")
    if(0 == size_) {
        WARN_MESSAGE("memory_system_allocate - No-op: size_ is 0.");
        ret = MEMORY_SYSTEM_SUCCESS;
        goto cleanup;
    }

    // Omitted

    ret = MEMORY_SYSTEM_SUCCESS;

cleanup:
    return ret;
}
```