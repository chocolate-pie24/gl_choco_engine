# GLCE Unit Test Code Implementation Guide

This document describes how to implement unit tests for the public APIs and private functions owned by a module.

## Failure Injection Mechanism for Testing

During testing, it is necessary to provide a mechanism that forces functions used inside the function under test to return a specified result code.
GLCE provides a dedicated failure injection mechanism so that this functionality can be used consistently across all modules.

This mechanism is declared in `test/include/test_controller.h` and implemented in `test/src/test_controller.c`.

`test_controller.h` exposes the following to external modules.

```c
/**
 * @brief Failure injection structure for testing various APIs
 *
 */
typedef struct test_call_control {
    uint32_t call_count;    /**< Number of API calls */
    uint32_t fail_on_call;  /**< Specifies on which call the API should fail (0: disabled and normal behavior, 1 or greater: force forced_result to be returned) */
    int forced_result;      /**< Return value to be forced when making the API fail (used by casting various result codes to int) */
} test_call_control_t;

/**
 * @brief Resets the fields of the failure injection structure so that failure injection is disabled
 *
 * @details After reset, the fields of the structure will have the following values:
 * - call_count = 0
 * - fail_on_call = 0
 * - forced_result = 0 (SUCCESS in many cases)
 *
 * @param[out] control_ Pointer to the structure to reset
 */
void test_call_control_reset(test_call_control_t* control_);
```

## Procedure for Creating Unit Test Code

### Creating a Test Header File

Create a test header file for the target module according to the following rules.

- The directory that stores the test header file should correspond to the hierarchy of the target module.
- The test header filename shall be `test_module_name.h`.

For example, if the target is `include/engine/core/memory/choco_memory.h`, then the test header file should be `test/include/engine/core/memory/test_choco_memory.h`.

The header file should contain the following items.

#### Failure Injection APIs

For all public APIs in the module that return result codes, define individual failure injection APIs.
The naming rule for these APIs is `test_foo_config_set`, where `foo` represents the name of the public API.

This API is used as follows.

```c
test_call_control_t config = { 0 };
test_call_control_reset(&config);

config.fail_on_call = 1;                // Fail on the first call
config.forced_result = (int)xxx_result; // Force API foo to return xxx module's result code
// For the call_count field, do not set it if you want to preserve the value managed internally by the module.
// It may be set if you do not need to preserve it.

test_foo_config_set(&config);   // Apply failure injection to API foo
```

#### Module Test API

Define an API that runs tests for all public APIs and private functions owned by the module.
The naming rule for this API is `test_xxx`, where `xxx` represents the module name.

#### Resetting Failure Injection Settings

Define an API that resets all failure injection settings owned by the module.
The naming rule for the reset API is `test_xxx_config_reset`, where `xxx` represents the module name.
In addition to failure injection settings, `test_xxx_config_reset` also resets all other test-related settings such as mock function test settings and test scenario settings.

A test header containing the above items will look like the following.

```c
#ifndef GLCE_TEST_PATH_MODULE_NAME_H
#define GLCE_TEST_PATH_MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_BUILD
#include "test_controller.h"    // Include the test infrastructure header

// Failure injection APIs for public APIs
// (prepare one for each public API that returns a result code)
void test_foo_config_set(const test_call_control_t* config_);   // Failure injection configuration API for API foo
void test_bar_config_set(const test_call_control_t* config_);   // Failure injection configuration API for API bar

// Reset all test configuration values inside the module
// (disables test behavior and also resets all settings other than failure injection,
// such as mock function test settings and test scenario settings)
void test_xxx_config_reset(void);

// Run tests for all functions owned by the module
// (public APIs and private functions)
void test_xxx(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
```

### Adding Test Implementations

Each test-related implementation should be written directly in the implementation file of the target module.
This allows the test code to directly access fields of the structures used by the module, keeping the test code simple.

#### Test Data Definitions and Prototype Declarations

Prepare a `TEST_BUILD` code block immediately after the include section of the module implementation file, and place the following items there.
To maintain consistency across all test functions, the order of these items must be unified across all modules.

- Includes used only during testing
- Declarations of failure injection structure instances for public APIs that return result codes (instance name: `s_test_config_xxx`, where `xxx` is the public API name)
- (If necessary) declarations of failure injection structure instances for private functions inside the module that return result codes (instance name: `s_test_config_xxx`, where `xxx` is the private function name)
- Prototypes for all test functions (test function name: `test_xxx`, where `xxx` is the target function name)
- The order of the prototypes must match the implementation order of the target functions

Each test function should be added after the implementations of the public APIs and private functions owned by the module.
The implementation order of the test functions is as follows.

- The test function that runs all test functions (the function declared in the test header)
- Individual unit test functions for each target function (implemented in the same order as the prototype declarations)

```c
#ifdef TEST_BUILD
// Includes used only during testing
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // for memset strcmp
#include "xxx/test_xxx.h"   // Test header for module xxx

// Public API test settings
static test_call_control_t s_test_config_foo;    /**< Test setting for API foo */
static test_call_control_t s_test_config_bar;    /**< Test setting for API bar */

// Private function test settings
// (add declarations only if necessary)
static test_call_control_t s_test_config_baz;    /**< Test setting for private function baz */

// Prototypes for all test functions
static void test_xxx(void);
#endif
```

### Failure Injection Handling in Public API Implementations

To make it possible to control result codes returned to upper layers, all public APIs that return result codes shall provide functionality to force a specified result code to be returned.
Since forced return values must work regardless of any error-handling path, this logic shall be placed at the beginning of the function.
For example, it should be implemented as follows (example: `memory_system_allocate`).

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
    IF_ARG_NULL_GOTO_CLEANUP(s_mem_sys_ptr, ret, MEMORY_SYSTEM_INVALID_ARGUMENT, rslt_to_str(MEMORY_SYSTEM_INVALID_ARGUMENT), "memory_system_allocate", "s_mem_sys_ptr")
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