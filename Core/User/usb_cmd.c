#include "usb_cmd.h"
#include "usbd_cdc_if.h"
#include "vcu_io.h"
#include "fsm_task.h"
#include <string.h>

#define CMD_BUF_SUZE  64
#define RESP_BUF_SIZE 64

static uint8_t  s_cmd_buf[CMD_BUF_SUZE];
static uint32_t s_cmd_buf_len = 0;

// Diagnostic counter: incremented each time CDC_Receive_FS delivers data.
volatile uint32_t g_usb_rx_count = 0;

// Response buffer written from ISR (dispatch_cmd), flushed from task context
// by usb_cmd_flush_response().  On Cortex-M4 (single-core, in-order stores),
// volatile ordering is sufficient — no DMB needed.
static volatile uint8_t  s_resp_buf[RESP_BUF_SIZE];
static volatile uint16_t s_resp_len = 0;  // 0 = no response pending

static void store_response(const uint8_t *data, uint16_t len) {
    if (s_resp_len != 0) return;  // prior response not yet sent; drop
    if (len > RESP_BUF_SIZE) len = RESP_BUF_SIZE;
    memcpy((uint8_t *)s_resp_buf, data, len);
    s_resp_len = len;             // written last: acts as publish flag
}

void usb_cmd_flush_response(void) {
    uint16_t len = s_resp_len;
    if (len == 0) return;
    if (CDC_Transmit_FS((uint8_t *)s_resp_buf, len) != USBD_BUSY) {
        s_resp_len = 0;
    }
    // If USBD_BUSY, leave s_resp_len set; task retries next iteration.
}

uint32_t dispatch_cmd(const uint8_t cmd, const uint8_t *payload, uint32_t len) {
    switch (cmd) {
        case CMD_ECHO:
            store_response(payload, (uint16_t)(len > RESP_BUF_SIZE ? RESP_BUF_SIZE : len));
            return 0;
        case CMD_SPOOF_SET:
            if (len >= sizeof(VcuInputs)) {
                vcu_spoof_inputs((const VcuInputs *)payload);
                return 0;
            }
            return 0xFFFFFFFF;
        case CMD_SPOOF_CLEAR:
            vcu_clear_spoof();
            return 0;
        case CMD_REQUEST_OUTPUTS: {
            static uint8_t resp[2 + sizeof(VcuOutputs)];
            resp[0] = 0x83;
            resp[1] = sizeof(VcuOutputs);
            memcpy(&resp[2], fsm_get_last_outputs(), sizeof(VcuOutputs));
            store_response(resp, (uint16_t)sizeof(resp));
            return 0;
        }
        case CMD_REQUEST_STATE: {
            static uint8_t resp[3];
            resp[0] = 0x84;
            resp[1] = 0x01;
            resp[2] = (uint8_t)fsm_get_state();
            store_response(resp, sizeof(resp));
            return 0;
        }
        case CMD_STEP:
            g_fsm_step_requested = true;
            return 0;
        case CMD_FAULT_INJECT:
            if (len >= 4) {
                uint32_t flags;
                memcpy(&flags, payload, 4);
                vcu_fault_inject(flags);
                return 0;
            }
            return 0xFFFFFFFF;
        case CMD_RESET:
            g_fsm_reset_requested = true;
            vcu_clear_spoof();
            return 0;
        default:
            return 0xFFFFFFFF;
    }
}

// A single command may be received in multiple USB packets,
// so we buffer until we have the complete frame.
// Frame: [CMD (1 byte)][LEN (1 byte)][payload (LEN bytes)]
uint32_t usb_cmd_rx(const uint8_t *buf, uint32_t len) {
    g_usb_rx_count++;
    for (uint32_t i = 0; i < len; i++) {
        if (s_cmd_buf_len < CMD_BUF_SUZE) {
            s_cmd_buf[s_cmd_buf_len++] = buf[i];
        }

        // wait until at least 2 bytes (CMD + LEN)
        if (s_cmd_buf_len < 2) {
            continue;
        }

        // wait until we have the complete frame (CMD + LEN + payload)
        uint32_t frame_len = s_cmd_buf[1] + 2;
        if (s_cmd_buf_len < frame_len) {
            continue;
        }

        // consume command
        dispatch_cmd(s_cmd_buf[0], &s_cmd_buf[2], s_cmd_buf[1]);
        s_cmd_buf_len -= frame_len;
        memmove(s_cmd_buf, &s_cmd_buf[frame_len], s_cmd_buf_len);
    }
    return 0;
}
