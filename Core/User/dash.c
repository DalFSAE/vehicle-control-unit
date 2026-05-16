#include "dash.h"
#include "can_bus.h"
#include "cmsis_os2.h"

static DashLedCmd_t s_leds  = {0};
static osMutexId_t  s_mutex = NULL;

void dash_init(void) {
    s_mutex = osMutexNew(NULL);
}

void dash_set_leds(const DashLedCmd_t *cmd) {
    if (cmd == NULL || s_mutex == NULL) return;
    osMutexAcquire(s_mutex, osWaitForever);
    s_leds = *cmd;
    osMutexRelease(s_mutex);
}

bool dash_tx_cmd(void) {
    if (s_mutex == NULL) return false;

    DashLedCmd_t snap;
    osMutexAcquire(s_mutex, osWaitForever);
    snap = s_leds;
    osMutexRelease(s_mutex);

    uint8_t frame[3] = {
        0,
        snap.imd_ok,
        snap.bms_ok,
    };
    return can_bus_transmit(CAN_ID_DASH_CMD, frame, sizeof(frame));
}

void dash_rx(uint32_t id, const uint8_t *data, size_t len) {
    (void)id;
    (void)data;
    (void)len;
}

const CanNode_t dash_node = {
    .name = "dash",
    .rx   = dash_rx,
};
