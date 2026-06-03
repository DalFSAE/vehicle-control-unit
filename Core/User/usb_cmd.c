#include "usb_cmd.h"
#include "usbd_cdc_if.h"
#include "vcu_io.h"
#include "fsm_task.h"
#include "cmsis_os2.h"
#include "log.h"
#include <string.h>

#define CMD_BUF_SUZE  64

static uint8_t  s_cmd_buf[CMD_BUF_SUZE];
static uint32_t s_cmd_buf_len = 0;

// Diagnostic counter: incremented each time CDC_Receive_FS delivers data.
volatile uint32_t g_usb_rx_count = 0;

void usb_cmd_flush_response(void) {
    // No-op: responses now go through log_queue_binary → s_log_queue.
}

uint32_t dispatch_cmd(const uint8_t cmd, const uint8_t *payload, uint32_t len) {
    switch (cmd) {
        case CMD_ECHO:
            log_queue_binary(payload, (uint16_t)(len > UART_BUF_SIZE ? UART_BUF_SIZE : len));
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
            log_queue_binary(resp, (uint16_t)sizeof(resp));
            return 0;
        }
        case CMD_REQUEST_STATE: {
            static uint8_t resp[3];
            resp[0] = 0x84;
            resp[1] = 0x01;
            resp[2] = (uint8_t)fsm_get_state();
            log_queue_binary(resp, sizeof(resp));
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