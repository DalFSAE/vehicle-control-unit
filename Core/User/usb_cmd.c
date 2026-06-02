#include "usb_cmd.h"
#include "usbd_cdc_if.h"
#include "string.h"

#define CMD_BUF_SUZE 64 // used to store incoming USB commands

static uint8_t s_cmd_buf[CMD_BUF_SUZE];
static uint32_t s_cmd_buf_len = 0; // reset on newline or overflow

enum {
    CMD_SPOOF_SET = 0x01,
    CMD_SPOOF_CLEAR = 0x02,
    CMD_ECHO = 0x45
};

uint32_t dispatch_cmd(const uint8_t cmd, const uint8_t *payload, uint32_t len) {
    switch (cmd) {
        case CMD_ECHO: // echo command for testing
            CDC_Transmit_FS((uint8_t *)payload, len);
            return 0;
        default:
            return 0xFFFFFFFF; // unknown command
    }
}

// A single command may be received in multiple USB packets, 
// so we buffer until we see a newline, then process the command.
// Frame: [CMD (1 byte)][LEN (1 byte)[payload (LEN bytes)]
uint32_t usb_cmd_rx(const uint8_t *buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) { 
        // overflow or newline resets the buffer
        if (s_cmd_buf_len < CMD_BUF_SUZE) {
            s_cmd_buf[s_cmd_buf_len++] = buf[i];
        }

        // wait until at least 2 bytes (CMD + LEN)
        if (s_cmd_buf_len < 2) {
            continue;
        }

        // consume command
        dispatch_cmd(s_cmd_buf[0], &s_cmd_buf[2], s_cmd_buf[1]);
        uint8_t frame_len = s_cmd_buf[1] + 2; // total frame length
        s_cmd_buf_len -= frame_len; // remove processed command from buffer
        memmove(s_cmd_buf, &s_cmd_buf[frame_len], s_cmd_buf_len); // shift remaining data to front

    }
    return 0;
}