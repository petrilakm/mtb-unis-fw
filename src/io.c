#include <avr/io.h>

#include "io.h"

void io_init() {
	DDRD = 0xFF; // outputs 7-0
	DDRC = 0xFF; // outputs 15-8
}

bool io_get_input_raw(uint8_t inum) {
}

uint16_t io_get_inputs_raw() {
}

void io_set_output_raw(uint8_t onum, bool state) {
}

void io_set_outputs_raw(uint16_t state) {
}
