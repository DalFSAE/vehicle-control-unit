#include "log.h"
#include "unity/unity.h"

bool log_write(const LogEvent_t *event) {
    (void)event;
    return true;
}

void setUp(void) {}
void tearDown(void) {}