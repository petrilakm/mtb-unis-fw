#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "io.h"

extern uint8_t config_safe_state[NO_OUTPUTS];
extern uint8_t config_inputs_delay[NO_OUTPUTS/2];
extern bool config_write;

// Warning: these functions take long time to execute
void config_load();
void config_save();

#endif
