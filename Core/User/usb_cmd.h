#pragma once
// usb_cmd.h
// Custom command interface for USB_DEVICE\App\usbd_cdc_if.c

#include <stdbool.h>
#include <stdint.h>

uint32_t usb_cmd_rx(const uint8_t *buf, uint32_t len);