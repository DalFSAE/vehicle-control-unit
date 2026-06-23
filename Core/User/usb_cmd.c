#include "usb_cmd.h"
#include "usbd_cdc_if.h"
#include "vcu_io.h"
#include "fsm_task.h"
#include "string.h"

#define CMD_BUF_SIZE 64

static uint8_t  s_cmd_buf[CMD_BUF_SIZE];
static uint32_t s_cmd_buf_len = 0;

// Handle a fully-assembled command frame. Called by usb_cmd_rx once a complete
// frame [CMD][LEN][payload] has been buffered. See docs/architecture/vcu-input-spoof.md
uint32_t dispatch_cmd(const uint8_t cmd, const uint8_t *payload, uint32_t len) {
    switch (cmd) {
        case CMD_ECHO:
            CDC_Transmit_FS((uint8_t *)payload, len);
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
            resp[0] = CMD_REPLY_OUTPUT;
            resp[1] = sizeof(VcuOutputs);
            memcpy(&resp[2], fsm_get_last_outputs(), sizeof(VcuOutputs));
            CDC_Transmit_FS(resp, sizeof(resp));
            return 0;
        }
        case CMD_REQUEST_STATE: {
            static uint8_t resp[3];
            resp[0] = CMD_REPLY_STATE;
            resp[1] = 0x01;
            resp[2] = (uint8_t)fsm_get_state();
            CDC_Transmit_FS(resp, sizeof(resp));
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
    for (uint32_t i = 0; i < len; i++) {
        if (s_cmd_buf_len < CMD_BUF_SIZE) {
            s_cmd_buf[s_cmd_buf_len++] = buf[i];
        }

        // wait until at least 2 bytes (CMD + LEN)
        if (s_cmd_buf_len < 2) {
            continue;
        }

        // reject frames too large to buffer to prevent deadlock where frame_len
        // exceeds CMD_BUF_SIZE and s_cmd_buf_len never reaches it
        uint32_t frame_len = s_cmd_buf[1] + 2;
        if (frame_len > CMD_BUF_SIZE) {
            s_cmd_buf_len = 0;
            continue;
        }
        // wait until we have the complete frame (CMD + LEN + payload)
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