#include <avr/io.h>
#include <util/delay.h>


#include "common.h"
#include "io.h"
volatile uint16_t output_shadow = 0;
uint16_t output_virt = 0;

volatile uint8_t dbg = 0;

void io_init() {
    PORTB = 0;
    PORTC = 0;
    PORTD = 0;
	DDRD = 0xFF; // outputs 0-7
	DDRC = 0xFF; // outputs 8-15
	DDRB = 0xF0; // servo outputs & servo power enable
	DDRE = 0x38; // servo outputs
	DDRG = (1 << PIN_LED_GREEN) | (1 << PIN_LED_RED) | (1 << PIN_LED_BLUE);
	PORTG = (1 << PIN_BUTTON); // button pull-up
}

bool io_get_input_raw(uint8_t inum) {
	return (io_get_inputs_raw() >> inum) & 0x1;
}

uint16_t io_get_inputs_raw(void) {
	uint16_t inputs;
	// bit position = input number
	inputs =  ((PINA	 )) << 8;
	inputs |= ((PINE >> 6) & 0x03);
	inputs |= ((PINF	 ) & 0xFC);
	return inputs;
}

void io_set_output_raw(uint8_t onum, bool state) {
	if (state)
		output_shadow |= (((uint16_t) 1) << onum);
	else
		output_shadow &= ~(((uint16_t) 1) << onum);
	io_set_outputs_raw();
}

void io_set_outputs_raw() {
	uint8_t low = output_shadow & 0xFF;
	uint8_t high = (output_shadow >> 8) & 0xFF;
	PORTD = low;
	PORTC = high;
}

void io_set_outputs_raw_mask(uint32_t state, uint32_t mask) {
	output_shadow = (output_shadow & (~mask));
	output_shadow |= (state & mask);
	io_set_outputs_raw();
}

uint16_t io_get_outputs_raw(void) {
	//return ((PORTC << 8) | PORTD);
	return output_shadow;
}

bool io_get_output_raw(uint8_t onum) {
	return (output_shadow >> onum) & 0x1;
}
