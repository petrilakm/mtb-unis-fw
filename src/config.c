#include <avr/eeprom.h>
#include <string.h>
#include "config.h"
#include "../lib/mtbbus.h"

uint8_t config_safe_state[NO_OUTPUTS];
uint8_t config_inputs_delay[NO_OUTPUTS/2];
bool config_write = false;
uint8_t config_mtbbus_speed;

#define EEPROM_ADDR_VERSION                ((uint8_t*)0x00)
#define EEPROM_ADDR_MTBBUS_SPEED           ((uint8_t*)0x01)
#define EEPROM_ADDR_INT_WDRF               ((uint8_t*)0x02)
#define EEPROM_ADDR_BOOT                   ((uint8_t*)0x03)
#define EEPROM_ADDR_BOOTLOADER_VER_MAJOR   ((uint8_t*)0x08)
#define EEPROM_ADDR_BOOTLOADER_VER_MINOR   ((uint8_t*)0x09)
#define EEPROM_ADDR_SAFE_STATE             ((void*)0x10)
#define EEPROM_ADDR_INPUTS_DELAY           ((void*)0x20)


void config_load(void) {
	uint8_t version = eeprom_read_byte(EEPROM_ADDR_VERSION);
	if (version == 0xFF) {
		// default EEPROM content → reset config
		config_mtbbus_speed = MTBBUS_SPEED_38400;
		memset(config_safe_state, 0, NO_OUTPUTS);
		memset(config_inputs_delay, 0, NO_OUTPUTS/2);
		config_save();
		return;
	}

	config_mtbbus_speed = eeprom_read_byte(EEPROM_ADDR_MTBBUS_SPEED);
	if (config_mtbbus_speed > MTBBUS_SPEED_115200)
		config_mtbbus_speed = MTBBUS_SPEED_38400;

	uint8_t boot = eeprom_read_byte(EEPROM_ADDR_BOOT);
	if (boot != CONFIG_BOOT_NORMAL)
		eeprom_write_byte(EEPROM_ADDR_BOOT, CONFIG_BOOT_NORMAL);

	eeprom_read_block(config_safe_state, EEPROM_ADDR_SAFE_STATE, NO_OUTPUTS);
	eeprom_read_block(config_inputs_delay, EEPROM_ADDR_INPUTS_DELAY, NO_OUTPUTS/2);
}

void config_save(void) {
	eeprom_update_byte(EEPROM_ADDR_VERSION, 1);
	eeprom_update_byte(EEPROM_ADDR_MTBBUS_SPEED, config_mtbbus_speed);
	eeprom_update_block(config_safe_state, EEPROM_ADDR_SAFE_STATE, NO_OUTPUTS);
	eeprom_update_block(config_inputs_delay, EEPROM_ADDR_INPUTS_DELAY, NO_OUTPUTS/2);
}

uint8_t input_delay(uint8_t input) {
	if (input >= NO_INPUTS)
		return 0;
	uint8_t both = config_inputs_delay[input/2];
	return (input%2 == 0) ? both & 0x0F : (both >> 4) & 0x0F;
}

void config_boot_fwupgd(void) {
	eeprom_update_byte(EEPROM_ADDR_BOOT, CONFIG_BOOT_FWUPGD);
}

void config_boot_normal(void) {
	eeprom_update_byte(EEPROM_ADDR_BOOT, CONFIG_BOOT_NORMAL);
}

void config_int_wdrf(bool value) {
	eeprom_update_byte(EEPROM_ADDR_INT_WDRF, value);
}

bool config_is_int_wdrf(void) {
	return eeprom_read_byte(EEPROM_ADDR_INT_WDRF) & 1;
}

uint16_t config_bootloader_version() {
	uint16_t version = (eeprom_read_byte(EEPROM_ADDR_BOOTLOADER_VER_MAJOR) << 8) |
	                   (eeprom_read_byte(EEPROM_ADDR_BOOTLOADER_VER_MINOR));
	return version == 0xFFFF ? 0x0101 : version;
}
