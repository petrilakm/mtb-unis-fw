#include <avr/eeprom.h>
#include "config.h"

uint8_t config_safe_state[NO_OUTPUTS];
uint8_t config_inputs_delay[NO_OUTPUTS/2];
bool config_write = false;

#define EEPROM_ADDR_VERSION       ((uint8_t*)0x00)
#define EEPROM_ADDR_SAFE_STATE    ((void*)0x10)
#define EEPROM_ADDR_INPUTS_DELAY  ((void*)0x20)

void config_load() {
	uint8_t version = eeprom_read_byte(EEPROM_ADDR_VERSION);
	if (version == 0xFF) {
		// default EEPROM content → reset config
		for (size_t i = 0; i < NO_OUTPUTS; i++)
			config_safe_state[i] = 0;
		for (size_t i = 0; i < NO_OUTPUTS/2; i++)
			config_inputs_delay[i] = 0;
		config_save();
		return;
	}

	eeprom_read_block(config_safe_state, EEPROM_ADDR_SAFE_STATE, NO_OUTPUTS);
	eeprom_read_block(config_inputs_delay, EEPROM_ADDR_INPUTS_DELAY, NO_OUTPUTS/2);
}

void config_save() {
	eeprom_write_byte(EEPROM_ADDR_VERSION, 1);
	eeprom_write_block(config_safe_state, EEPROM_ADDR_SAFE_STATE, NO_OUTPUTS);
	eeprom_write_block(config_inputs_delay, EEPROM_ADDR_INPUTS_DELAY, NO_OUTPUTS/2);
}

uint8_t input_delay(uint8_t input) {
	if (input >= NO_INPUTS)
		return 0;
	uint8_t both = config_inputs_delay[input/2];
	return (input%2 == 0) ? both & 0x0F : (both >> 4) & 0x0F;
}
