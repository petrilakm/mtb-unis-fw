#include <stddef.h>
#include "inputs.h"
#include "io.h"
#include "config.h"

uint16_t inputs_logic_state= 0;
uint16_t inputs_debounced_state = 0;
uint16_t inputs_old = 0;

#define DEBOUNCE_THRESHOLD 100 // 10 ms
uint8_t _inputs_debounce_counter[NO_INPUTS] = {0, };

uint8_t _inputs_fall_counter[NO_INPUTS] = {0, };

void inputs_debounce_update() {
	uint16_t state = io_get_inputs_raw();
	for (size_t i = 0; i < NO_INPUTS; i++) {
		if (state & 0x01) { // state == 1 → logical 0
			if (_inputs_debounce_counter[i] > 0) {
				_inputs_debounce_counter[i]--;
				// If you need evens in future, you must enter if below only if
				// inputs_debounced_state[i] == 1
				if (_inputs_debounce_counter[i] == 0) {
					inputs_debounced_state &= ~(1 << i);
					_inputs_fall_counter[i] = input_delay(i)*10;
					if (_inputs_fall_counter[i] == 0)
						inputs_logic_state &= ~(1 << i);
				}
			}
		} else {
			if (_inputs_debounce_counter[i] < DEBOUNCE_THRESHOLD) {
				_inputs_debounce_counter[i]++;
				// If you need evens in future, you must enter if below only if
				// inputs_debounced_state[i] == 0
				if (_inputs_debounce_counter[i] == DEBOUNCE_THRESHOLD) {
					inputs_debounced_state |= (1 << i);
					inputs_logic_state |= (1 << i); // no fall
				}
			}
		}

		state >>= 1;
	}
}

void inputs_fall_update() {
	for (size_t i = 0; i < NO_INPUTS; i++) {
		if (_inputs_fall_counter[i] > 0) {
			_inputs_fall_counter[i]--;
			if (_inputs_fall_counter[i] == 0)
				inputs_logic_state &= ~(1 << i);
		}
	}
}
