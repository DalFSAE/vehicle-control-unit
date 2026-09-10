#include "dms_logging.h"
#include "app_main.h"

// #include "usbd_cdc_if.h" 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// TODO: Incorporate CAN into logging
#define CAN_LOG_BUF_SIZE 128
#define CAN_LOG_MSG_ID   0x7E0

extern CAN_HandleTypeDef hcan1;

void dms_printf(const char *format, ...) { 
    char log_buf[CAN_LOG_BUF_SIZE];
    
    // 1. Format the variadic arguments into a single string buffer
    va_list args;
    va_start(args, format);
    int len = vsnprintf(log_buf, sizeof(log_buf), format, args);
    va_end(args);

    if (len <= 0) {
        return;
    }

    // Cap length to buffer capacity
    if (len >= CAN_LOG_BUF_SIZE) {
        len = CAN_LOG_BUF_SIZE - 1;
    }

    // 2. Configure CAN Tx Header
    CAN_TxHeaderTypeDef txHeader;
    uint32_t txMailbox;

    txHeader.StdId = CAN_LOG_MSG_ID;
    txHeader.IDE   = CAN_ID_STD;
    txHeader.RTR   = CAN_RTR_DATA;
    txHeader.TransmitGlobalTime = DISABLE;

    // 3. Chunk the formatted string into 8-byte CAN payloads
    int bytes_remaining = len;
    int offset = 0;

    while (bytes_remaining > 0) {
        uint8_t chunk_size = (bytes_remaining > 8) ? 8 : (uint8_t)bytes_remaining;
        uint8_t txData[8] = {0};

        // Copy up to 8 bytes into the CAN payload
        memcpy(txData, &log_buf[offset], chunk_size);
        txHeader.DLC = chunk_size;

        // Wait until a TX mailbox is available to prevent dropping frames
        while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0) {
            // Optional timeout or watchdog kick
        }

        // Send the CAN frame
        if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) != HAL_OK) {
            break; // Transmission failed
        }

        offset += chunk_size;
        bytes_remaining -= chunk_size;
    }
}
