#include <stddef.h> // for NULL
#include <stdlib.h> // for malloc TODO: remove this!!
#include <string.h> // for memset

#include "application/application.h"

#include "engine/core/message.h"

typedef struct app_state {
    int rubbish;    // 一時的にビルド通すため
} app_state_t;

static app_state_t* s_app_state = NULL;

// TODO: oc_choco_malloc + テスト
app_err_t application_create(void) {
    app_err_t ret = APPLICATION_RUNTIME_ERROR;
    app_state_t* tmp = NULL;

    // Preconditions
    if(NULL != s_app_state) {   // TODO: CHECK_NOT_VALID_GOTO_CLEANUP()
        ERROR_MESSAGE("application_create(RUNTIME_ERROR) - Application state is already initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }

    // Simulation
    tmp = (app_state_t*)malloc(sizeof(*tmp)); // TODO: choco_malloc
    if(NULL == tmp) {   // TODO: CHECK_ALLOC_ERR_GOTO_CLEANUP()
        ERROR_MESSAGE("application_create(NO_MEMORY) - Failed to allocate app_state memory.\n");
        ret = APPLICATION_NO_MEMORY;
        goto cleanup;
    }
    memset(tmp, 0, sizeof(*tmp));

    // commit
    s_app_state = tmp;

    // begin launch all systems.
    // end launch all systems.

    ret = APPLICATION_SUCCESS;

cleanup:
    return ret;
}

// TODO: test
void application_destroy(void) {
    if(NULL == s_app_state) {
        goto cleanup;
    }

    // begin cleanup all systems.
    // end cleanup all systems.

    free(s_app_state);  // TODO: choco_free
    s_app_state = NULL;
cleanup:
    return;
}

app_err_t application_run(void) {
    app_err_t ret = APPLICATION_SUCCESS;
    if(NULL == s_app_state) {
        ERROR_MESSAGE("application_run(APPLICATION_RUNTIME_ERROR) - Application is not initialized.\n");
        ret = APPLICATION_RUNTIME_ERROR;
        goto cleanup;
    }
    // while(APPLICATION_SUCCESS == ret) {
    // }

cleanup:
    return ret;
}
