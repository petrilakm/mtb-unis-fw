#include <stdbool.h>
#include "outputs.h"
#include "io.h"
#include "scom.h"

const uint8_t _flicker_periods[] = { // in times of calls to outputs_update (10 ms)
	10, // invalid
	50, // 1 Hz
	25, // 2 Hz
	16, // ~ 3 Hz
	12, // ~ 4 Hz
	10, // 5 Hz
	5,  // 10 Hz
	100, // 33 tick / min
	50, // 66 tick / min
};

// Each output has it's flickering counter to avoid neccessity for single
// lowest-common-multiplier counter (LCM is too big)
uint8_t _flicker_counters[NO_OUTPUTS] = {0, };
bool _flicker_enabled[NO_OUTPUTS] = {false, };

// State according to ‹protocol›
// ‹https://github.com/kmzbrnoI/mtbbus-protocol/blob/master/modules/uni.md›
uint8_t _outputs_state[NO_OUTPUTS] = {0, };

bool outputs_need_apply = false;

void outputs_apply_state() {
	if (!outputs_need_apply)
		return;
	outputs_need_apply = false;
// DEBUG !!!
// pokud je ve for i=>12, tak se procesor zasekne, Watchdog nepomůže !
// tak na i úplně nesejde 
// DEBUG !!!
	for (int i = 0; i < NO_OUTPUTS; i++) {
		//io_set_output_raw(i, _outputs_state[i]);
		_flicker_enabled[i] = _outputs_state[i] & 0x40;
		if (!_flicker_enabled[i]) {
			_flicker_counters[i] = 0;
			if (_outputs_state[i] & 0x80) { // S-COM
				scom_output(i, _outputs_state[i] & 0x7F);
			} else { // plain output
				//scom_disable_output(i);
				scom_output(i, -1);
				io_set_output_raw(i, _outputs_state[i]);
			}
		}
	}
}

void outputs_set_zipped(uint8_t data[], size_t length) {
	if (length < 6)
		return;

	uint16_t full_mask = data[1] | (data[0] << 8);
	//uint32_t bin_state = data[5] | (data[4] << 8) | (((uint32_t) data[3]) << 16) | (((uint32_t) data[2]) << 24);
	uint16_t bin_state = data[5] | (data[4] << 8);
	size_t bytei = 6;

	for (uint8_t i = 0; i < NO_OUTPUTS; i++) {
		if (((full_mask) & 1) == 0) {
			_outputs_state[i] = (bin_state) & 1;
		} else {
			if (bytei < length) {
				_outputs_state[i] = data[bytei];
				bytei++;
			}
		}
		full_mask >>= 1;
		bin_state >>= 1;
	}

	output_virt = data[3] | (data[2] << 8);
}

void outputs_set_full(uint8_t data[NO_OUTPUTS]) {
	for (size_t i = 0; i < NO_OUTPUTS; i++)
		_outputs_state[i] = data[i];
	outputs_need_apply = true;
	//outputs_apply_state();
}

void outputs_update() {
	for (size_t i = 0; i < NO_OUTPUTS; i++) {
		if (!_flicker_enabled[i])
			continue;

		_flicker_counters[i]++;
		uint8_t flicker_type = _outputs_state[i] & 0x0F;
		if (flicker_type >= sizeof(_flicker_periods)/sizeof(*_flicker_periods))
			flicker_type = 0; // error: unknown flicker period → default flicker period
		if (_flicker_counters[i] == _flicker_periods[flicker_type]) {
			io_set_output_raw(i, true);
		} else if (_flicker_counters[i] >= 2*_flicker_periods[flicker_type]) {
			io_set_output_raw(i, false);
			_flicker_counters[i] = 0;
		}
	}
}
