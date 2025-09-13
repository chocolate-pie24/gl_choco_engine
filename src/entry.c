#include <stdio.h>

#include "application/application.h"

int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    fprintf(stdout, "Build mode: RELEASE.\n");
#endif
#ifdef DEBUG_BUILD
    fprintf(stdout, "Build mode: DEBUG.\n");
#endif
#ifdef TEST_BUILD
    fprintf(stdout, "Build mode: TEST.\n");
#endif

    const app_err_t app_create_result = application_create();
    if(APPLICATION_SUCCESS != app_create_result) {
        goto cleanup;
    } else {
        fprintf(stdout, "Application created successfully.\n");
    }

    const app_err_t app_run_result = application_run();
    if(APPLICATION_SUCCESS != app_run_result) {
        goto cleanup;
    } else {
        fprintf(stdout, "Application executed successfully.\n");
    }

cleanup:
    application_destroy();
    return 0;
}
