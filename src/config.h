#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "io.h"

extern uint8_t config_safe_state[NO_OUTPUTS];
extern uint8_t config_inputs_delay[NO_OUTPUTS/2];
extern bool config_write;
extern uint8_t config_mtbbus_speed;

// Warning: these functions take long time to execute
void config_load();
void config_save();

uint8_t input_delay(uint8_t input);

#define CONFIG_MODULE_TYPE 0x15
#define CONFIG_FW_MAJOR 0
#define CONFIG_FW_MINOR 1
#define CONFIG_PROTO_MAJOR 4
#define CONFIG_PROTO_MINOR 0

#endif
