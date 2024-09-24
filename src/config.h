#ifndef _CONFIG_H_
#define _CONFIG_H_

/* General configuration of MTB-UNI module
 */

#include <stdint.h>
#include <stdbool.h>
#include "io.h"

extern uint8_t config_safe_state[NO_OUTPUTS_ALL];
extern uint8_t config_inputs_delay[NO_OUTPUTS/2];
extern bool config_write;
extern uint8_t config_mtbbus_speed;
extern uint8_t config_mtbbus_addr;
extern uint8_t config_servo_enabled;
extern uint8_t config_servo_position[NO_SERVOS*2];
extern uint8_t config_servo_speed[NO_SERVOS];

// Warning: these functions take long time to execute
void config_load(void);
void config_save(void);

void config_boot_fwupgd(void);
void config_boot_normal(void);

void config_int_wdrf(bool value);
bool config_is_int_wdrf(void);

uint16_t config_bootloader_version(void);

uint8_t input_delay(uint8_t input);
void set_address(uint8_t address);

#define CONFIG_MODULE_TYPE 0x50
#define CONFIG_FW_MAJOR 1
#define CONFIG_FW_MINOR 4
#define CONFIG_PROTO_MAJOR 4
#define CONFIG_PROTO_MINOR 1

#define CONFIG_BOOT_FWUPGD 0x01
#define CONFIG_BOOT_NORMAL 0x00

#endif
