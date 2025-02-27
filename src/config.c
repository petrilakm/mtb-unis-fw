#include <avr/eeprom.h>
#include <string.h>
#include "config.h"
#include "../lib/mtbbus.h"

uint8_t config_safe_state[NO_OUTPUTS_ALL];	// 28 bytes
uint8_t config_inputs_delay[NO_OUTPUTS/2];	// 8 bytes
bool config_write = false;
uint8_t config_mtbbus_speed;
uint8_t config_mtbbus_addr;
uint8_t config_servo_enabled;	// 1 byte
uint8_t config_servo_position[NO_SERVOS*2];	// 12 bytes
uint8_t config_servo_speed[NO_SERVOS];	// 6 bytes


#define EEPROM_ADDR_VERSION                ((uint8_t*)0x00)
#define EEPROM_ADDR_MTBBUS_SPEED           ((uint8_t*)0x01)
#define EEPROM_ADDR_INT_WDRF               ((uint8_t*)0x02)
#define EEPROM_ADDR_BOOT                   ((uint8_t*)0x03)
#define EEPROM_ADDR_MTBBUS_ADDR            ((uint8_t*)0x04)
#define EEPROM_ADDR_BOOTLOADER_VER_MAJOR   ((uint8_t*)0x08)
#define EEPROM_ADDR_BOOTLOADER_VER_MINOR   ((uint8_t*)0x09)
#define EEPROM_ADDR_SAFE_STATE             ((uint8_t*)0x10)
#define EEPROM_ADDR_INPUTS_DELAY           ((uint8_t*)0x30)
#define EEPROM_ADDR_SERVO_ENABLED          ((uint8_t*)0x38)
#define EEPROM_ADDR_SERVO_POSITION         ((uint16_t*)0x39)
#define EEPROM_ADDR_SERVO_SPEED            ((uint8_t*)0x45)
#define EEPROM_ADDR_EMPTY                  ((void*)0x57)

void config_load(void) {
	uint8_t version = eeprom_read_byte(EEPROM_ADDR_VERSION);
	if (version == 0xFF) {
		// default EEPROM content → reset config
		config_mtbbus_speed = MTBBUS_SPEED_38400;
		config_mtbbus_addr = 1;
		memset(config_safe_state, 0, NO_OUTPUTS);
		memset(config_inputs_delay, 0, NO_INPUTS/2);
		while (!config_save()); // loop until everything saved
		return;
	}

	config_mtbbus_speed = eeprom_read_byte(EEPROM_ADDR_MTBBUS_SPEED);
	if (config_mtbbus_speed > MTBBUS_SPEED_MAX)
		config_mtbbus_speed = MTBBUS_SPEED_38400;

	config_mtbbus_addr = eeprom_read_byte(EEPROM_ADDR_MTBBUS_ADDR);

	uint8_t boot = eeprom_read_byte(EEPROM_ADDR_BOOT);
	if (boot != CONFIG_BOOT_NORMAL)
		eeprom_write_byte(EEPROM_ADDR_BOOT, CONFIG_BOOT_NORMAL);

	eeprom_read_block(config_safe_state,     EEPROM_ADDR_SAFE_STATE,     NO_OUTPUTS_ALL);
	eeprom_read_block(config_inputs_delay,   EEPROM_ADDR_INPUTS_DELAY,   NO_INPUTS/2);
	eeprom_read_block(config_servo_position, EEPROM_ADDR_SERVO_POSITION, NO_SERVOS*2);
	eeprom_read_block(config_servo_speed,    EEPROM_ADDR_SERVO_SPEED,    NO_SERVOS);
	for (size_t i = 0; i < NO_SERVOS; i++)
		if (config_servo_speed[i] == 0) config_servo_speed[i] = 10;
	config_servo_enabled = eeprom_read_byte(EEPROM_ADDR_SERVO_ENABLED);
}

bool config_save(void) {
	// Saves only 1 byte at a time, so this function never blocks for a long time (EEPROM busy wait).
	// Returns true iff all data have been succesfully saved.
	// If false is returned, call 'config_save' again to really save all data.

	if (!eeprom_is_ready())
		return false;
	eeprom_update_byte(EEPROM_ADDR_VERSION, 1);
	if (!eeprom_is_ready())
		return false;
	eeprom_update_byte(EEPROM_ADDR_MTBBUS_SPEED, config_mtbbus_speed);
	if (!eeprom_is_ready())
		return false;
	eeprom_update_byte(EEPROM_ADDR_MTBBUS_ADDR,   config_mtbbus_addr);
	if (!eeprom_is_ready())
		return false;
	eeprom_update_byte(EEPROM_ADDR_SERVO_ENABLED, config_servo_enabled);
	if (!eeprom_is_ready())
		return false;
	for (uint8_t i = 0; i < NO_OUTPUTS_ALL; i++) {
		if (!eeprom_is_ready())
			return false;
		eeprom_update_byte(EEPROM_ADDR_SAFE_STATE+i, config_safe_state[i]);
	}

	for (uint8_t i = 0; i < NO_INPUTS/2; i++) {
		if (!eeprom_is_ready())
			return false;
		eeprom_update_byte(EEPROM_ADDR_INPUTS_DELAY+i, config_inputs_delay[i]);
	}

	for (uint8_t i = 0; i < NO_SERVOS*2; i++) {
		if (!eeprom_is_ready())
			return false;
		eeprom_update_byte(EEPROM_ADDR_SERVO_POSITION+i, config_servo_position[i]);
	}

	for (uint8_t i = 0; i < NO_SERVOS; i++) {
		if (!eeprom_is_ready())
			return false;
		eeprom_update_byte(EEPROM_ADDR_SERVO_SPEED+i, config_servo_speed[i]);
	}

	return eeprom_is_ready(); // true iff no write done
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
