#include <avr/io.h>
#include <util/delay.h>

#include "io.h"
#include "common.h"

uint32_t output_shadow = 0;

void io_init() {
               /*
	DDRD = 0xFF; // outputs 7-0
	DDRC = 0xFF; // outputs 15-8
	DDRB = (1 << PIN_LED_RED) | (1 << PIN_LED_GREEN);
	DDRG = (1 << PIN_LED_BLUE);
	PORTG = (1 << PIN_BUTTON); // button pull-up
	PORTE = (1 << PIN_UART_RX); // RX pull-up
              */
	DDRD = 0xFF; // outputs 0-7
	DDRC = 0xFF; // outputs 8-15
	DDRB = 0xF0; // servo outputs
              DDRE = 0x38; // servo outputs
	DDRG = (1 << PIN_LED_RED) | (1 << PIN_LED_BLUE);
	PORTG = (1 << PIN_BUTTON); // button pull-up
}

bool io_get_input_raw(uint8_t inum) {
	return (io_get_inputs_raw() >> inum) & 0x1;
}

uint16_t io_get_inputs_raw() {
/*
	uint16_t inputs = bit_reverse(PINF);
	inputs |= ((PINE >> 3) & 0x1F) << 8;
	inputs |= ((PINB >> INPUT_13) & 0x1) << 13;
	inputs |= ((PINB >> INPUT_14) & 0x1) << 14;
	inputs |= ((PINB >> INPUT_15) & 0x1) << 15;
              */
              uint16_t inputs;
              inputs =  ((PINA     )) << 8;
              inputs |= ((PINE >> 6) & 0x03);
              inputs |= ((PINF     ) & 0xFC);
	return inputs;
}

void io_set_output_raw(uint8_t onum, bool state) {
	if (state)
		output_shadow |= (1 << onum);
	else
		output_shadow &= ~(1 << onum);
	io_set_outputs_raw(output_shadow);
}

void io_set_outputs_raw(uint32_t state) {
	output_shadow = state;
	uint8_t low = output_shadow & 0xFF;
	uint8_t high = (output_shadow >> 8) & 0xFF;
	PORTD = low;
	PORTC = high;
}

void io_set_outputs_raw_mask(uint32_t state, uint32_t mask) {
	state = (state & mask) | (output_shadow & (~mask));
	io_set_outputs_raw(state);
}

uint32_t io_get_outputs_raw() {
	//return ((PORTC << 8) | PORTD);
	return output_shadow;
}

bool io_get_output_raw(uint8_t onum) {
	return (output_shadow >> onum) & 0x1;
}
