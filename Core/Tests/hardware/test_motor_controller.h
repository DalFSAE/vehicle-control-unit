#pragma once

#include "hardware_test_runner.h"

void test_mc_cmd_cache_roundtrip(void);
void test_mc_cmd_encoding_loopback(void);
void test_mc_heartbeat_received(void);
void test_mc_vsm_is_ready(void);
void test_mc_no_active_faults(void);

BootResult_t run_mc_tests(void);
