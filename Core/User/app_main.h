 #ifndef APP_MAIN_H
 #define APP_MAIN_H

#include "stm32f4xx_hal.h"

typedef enum {
	SM_OKAY				= 0x00u,
	SM_FAIL			    = 0x01u,
	SM_REPEAT			= 0x02u,
	SM_DIR_FORWARD		= 0x03u,
	SM_DIR_REVERSE		= 0x04u,
	SM_VEHICLE_STOPPED	= 0x05u,
	SM_CHANGE_MAP		= 0x06u,
	SM_ADC_DATA_READY	= 0x07u,
    SM_ERROR            = 0Xffu
} ret_codes_t;

/* called when entering state */
int entry_state(void);
int neutral_state(void);
int forward_state(void);
int reverse_state(void);
int end_state(void);


void app_config();
void app_main();

 #endif