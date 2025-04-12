#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "stdbool.h"
// #include "usbd_cdc_if.h"

#include "app_main.h"
#include "sensor_control.h"
#include "io_control.h"
#include "dms_logging.h"

#include "stdio.h"

#define EXIT_STATE end
#define ENTRY_STATE entry

typedef enum { entry, neutral, forward, reverse, end }state_codes_t;
/**
 * @brief      Maps a state to it's state transition function, which should be called
 *             when the state transitions into this state.
 * @warning    This has to stay in sync with the state_codes_t enum!
 */
int (*state[])(void) = {
    entry_state, 
    neutral_state, 
    forward_state, 
    reverse_state, 
    end_state
};


struct transition {
	state_codes_t src_state;
	ret_codes_t ret_code;
	state_codes_t dst_state;
};

// Defines the State, Input, and Next state
struct transition state_transitions[] = {
    {entry,         SM_OKAY,                neutral},
	{entry,         SM_FAIL,                entry  },
	{neutral,       SM_DIR_FORWARD,  	    forward},
	{neutral,       SM_DIR_REVERSE,         reverse},
	{neutral,       SM_OKAY,                neutral},
	{forward,       SM_OKAY,                forward},
	{forward,       SM_FAIL,                forward},
	{forward,       SM_CHANGE_MAP,          forward},
	{forward,       SM_VEHICLE_STOPPED,     neutral},
	{forward,       SM_ADC_DATA_READY,      forward},
	{reverse,       SM_OKAY,                reverse},
	{reverse,       SM_FAIL,                forward},
	{reverse,       SM_VEHICLE_STOPPED,     neutral},
};

void statusLedsTask(void *argument) {
    (void)argument;
    
    int debug = 0;
    
    for(;;) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
        
        // testing relays...
        if (debug) {
            relay_toggle(RELAY_EN_0);
        }


        osDelay(250);
    }
}

void app_config(){
	// HAL_TIM_Base_Start(&htim2);
	// HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    return;
}

int entry_state(void){    
    return SM_OKAY;
}

int neutral_state(void){
    // Check if forward or reverse selected
    enable_throttle(false);
    return SM_DIR_FORWARD;
}

int forward_state(void){
    // Check if neutral or reverse sw is selected
    enable_throttle(true);
    return SM_OKAY;
}

int reverse_state(void){
    return SM_OKAY;
}

int end_state(void){
    return SM_OKAY;
}

/**
 * @brief Look up the next state based on the current state and return code.
 * @param cur_state The current state of the state machine.
 * @param rc The return code indicating the result of the previous state operation.
 * @return The next state of the state machine if a valid transition is found,
 *         otherwise SM_ERROR.
 */
state_codes_t lookup_transitions(state_codes_t cur_state, ret_codes_t rc){
	for (uint16_t i = 0; i < sizeof(state_transitions) / sizeof(state_transitions[0]); i++) {
		if (state_transitions[i].src_state == cur_state && state_transitions[i].ret_code == rc) {
            return state_transitions[i].dst_state; // Return the next state
		}
    }
    // Return an error code indicating that no matching transition was found
	return (state_codes_t)SM_ERROR;
}

void stateMachineTask(void *argument){
    (void)argument; // fixes compiler warning 

    state_codes_t cur_state = ENTRY_STATE;
	ret_codes_t rc;
	int (*state_fun)(void);
    
    dms_printf("[DEBUG] State machine task started\n\r");

    for(;;) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
	    state_fun = state[cur_state];       
	    rc = state_fun();                   // runs the corresponding state function, and returns the state code
	    cur_state = lookup_transitions(cur_state, rc);
        
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);

        enable_throttle(false);

        
        osDelay(10);
    }
}