#define LOG_MODULE LOG_SRC_CAN
#include "log.h"

#include "can_bus.h"
#include "node.h"
#include "motor_controller.h"
#include "can0_powertrain.h"

#include <string.h>

static CAN_HandleTypeDef *s_hcan = NULL;
static CanRxHook_t        s_rx_hook = NULL;

// Init
void can_bus_init(CAN_HandleTypeDef *hcan, uint32_t mode) {
    s_hcan = hcan;

    // If running, abort any pending TX mailboxes and stop cleanly before
    // switching mode. HAL_CAN_Init entering init-mode does NOT flush TX
    // mailboxes their TXRQ bits survive and retransmit once Start() exits
    // init-mode, which would loop stale frames back in loopback mode.
    if (hcan->State == HAL_CAN_STATE_LISTENING) {
        HAL_CAN_AbortTxRequest(hcan, CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2);
        HAL_CAN_Stop(hcan);
    }

    hcan->Init.Mode = mode;
    HAL_CAN_Init(hcan);

    CAN_FilterTypeDef f = {
        .FilterIdHigh = 0x0000,
        .FilterIdLow = 0x0000,
        .FilterMaskIdHigh = 0x0000,
        .FilterMaskIdLow = 0x0000,
        .FilterFIFOAssignment = CAN_RX_FIFO0,
        .FilterBank = 0,
        .FilterMode = CAN_FILTERMODE_IDMASK,
        .FilterScale = CAN_FILTERSCALE_32BIT,
        .FilterActivation = CAN_FILTER_ENABLE,
        .SlaveStartFilterBank = 14,
    };
    HAL_CAN_ConfigFilter(hcan, &f);
    HAL_CAN_Start(hcan);

    // Drain whatever landed in the FIFO before the notification is armed so
    // that the ISR only fires for frames sent after this point.
    {
        CAN_RxHeaderTypeDef h;
        uint8_t             d[8];
        while (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &h, d) == HAL_OK) {
        }
    }

    HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
}

// TX
bool can_bus_transmit(uint32_t id, const uint8_t *data, uint8_t len) {
    if (s_hcan == NULL)
        return false;

    CAN_TxHeaderTypeDef hdr = {
        .StdId = id,
        .ExtId = 0,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .DLC = len,
        .TransmitGlobalTime = DISABLE,
    };

    uint32_t mailbox;
    return HAL_CAN_AddTxMessage(s_hcan, &hdr, (uint8_t *)data, &mailbox) == HAL_OK;
}

// Test hook
void can_bus_set_rx_hook(CanRxHook_t hook) {
    s_rx_hook = hook;
}

// RX dispatch called from ISR
static void can_bus_dispatch(uint32_t id, const uint8_t *data, size_t len) {
    if (s_rx_hook != NULL) {
        s_rx_hook(id, data, len);
    }

    switch (id) {
        case CAN0_POWERTRAIN_M170_INTERNAL_STATES_FRAME_ID:
        case CAN0_POWERTRAIN_M171_FAULT_CODES_FRAME_ID:
        case CAN0_POWERTRAIN_M172_TORQUE_AND_TIMER_INFO_FRAME_ID: inverter_rx(id, data, len); break;
        default: break;
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    if (hcan != s_hcan)
        return;

    CAN_RxHeaderTypeDef hdr;
    uint8_t             data[8];

    while (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data) == HAL_OK) {
        can_bus_dispatch(hdr.StdId, data, hdr.DLC);
    }
}
