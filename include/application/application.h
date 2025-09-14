#ifndef APPLICATION_H
#define APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APPLICATION_SUCCESS,
    APPLICATION_NO_MEMORY,
    APPLICATION_RUNTIME_ERROR,
    APPLICATION_UNDEFINED_ERROR,
} app_err_t;

app_err_t application_create(void);

void application_destroy(void);

app_err_t application_run(void);

#ifdef __cplusplus
}
#endif
#endif
