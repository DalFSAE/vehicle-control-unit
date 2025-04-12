#include "dms_logging.h"

// #include "usbd_cdc_if.h" 
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define USB_BUF_SIZE 256


void dms_printf(const char *format, ...){ 
    
    /* 
    // TODO: MPU will crash if two print statements are called dirrectly after each other
    static char USB_buf[USB_BUF_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(USB_buf, USB_BUF_SIZE, format, args);
    va_end(args);

    // Transmit the formatted string over USB
    uint16_t len = strlen(USB_buf);
    CDC_Transmit_FS((uint8_t*)USB_buf, len);
    */
    // HAL_Delay(1);

}