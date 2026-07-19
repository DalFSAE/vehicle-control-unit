#pragma once
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOOT_OK = 0,
    BOOT_ERR_PRE_BOOT_TESTS,
    BOOT_ERR_POST_BOOT_TESTS,
    BOOT_ERR_TASK_CREATE,
} BootStatus_t;

uint32_t app_init(void);
void     app_post_boot(void);
void     app_create_tasks(void);
void     app_error_handler(BootStatus_t status);

#ifdef __cplusplus
}
#endif
