#pragma once

// This thead controls the hardware logic for DMS-27
// To improve testablity, only the top level task should access hardware
void fsm_task(void *arg);
