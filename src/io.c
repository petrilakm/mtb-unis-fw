#include <avr/io.h>

#include "io.h"
#include "common.h"

void io_init() {
	DDRD = 0xFF; // outputs 7-0
	DDRC = 0xFF; // outputs 15-8
	DDRB |= 0xC0; // LEDs PB6 (red), PB7 (green)
	DDRG |= 0x08; // LED PG3 (blue)
	PORTG |= 0x10; // button pull-up
}

bool io_get_input_raw(uint8_t inum) {
	return (io_get_inputs_raw() >> inum) & 0x1;
}

uint16_t io_get_inputs_raw() {
	uint16_t inputs = bit_reverse(PINF);
	inputs |= ((PINE >> 3) & 0x1F) << 8;
	inputs |= ((PINB >> INPUT_13) & 0x1) << 13;
	inputs |= ((PINB >> INPUT_14) & 0x1) << 14;
	inputs |= ((PINB >> INPUT_15) & 0x1) << 15;
	return inputs;
}

void io_set_output_raw(uint8_t onum, bool state) {
	uint16_t outputs = io_get_outputs_raw();
	if (state)
		outputs |= (1 << onum);
	else
		outputs &= ~(1 << onum);
	io_set_outputs_raw(outputs);
}

void io_set_outputs_raw(uint16_t state) {
	io_set_outputs_raw_mask(state, 0xFFFF);
}

void io_set_outputs_raw_mask(uint16_t state, uint16_t mask) {
	state = (state & mask) | (io_get_outputs_raw() & (~mask));
	uint8_t low = state & 0xFF;
	uint8_t high = (state >> 8) & 0xFF;
	PORTD = bit_reverse(low);
	PORTC = bit_reverse(high);
}

uint16_t io_get_outputs_raw() {
	return (bit_reverse(PORTC) << 8) | bit_reverse(PORTD);
}

bool io_get_output_raw(uint8_t onum) {
	return (io_get_outputs_raw() >> onum) & 0x1;
}
