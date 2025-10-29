#include <stdbool.h>
#include <string.h>
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
uint8_t _flicker_counters[NO_OUTPUTS_ALL] = {0, };
bool _flicker_enabled[NO_OUTPUTS_ALL] = {false, };

// State according to ‹protocol›
// ‹https://github.com/kmzbrnoI/mtbbus-protocol/blob/master/modules/uni.md›
uint8_t _outputs_state[NO_OUTPUTS_ALL] = {0, };

bool outputs_need_apply = false;

void outputs_apply_state() {
	if (!outputs_need_apply)
		return;
	outputs_need_apply = false;

	uint16_t plain_mask = 0;
	uint16_t plain_state = 0;

	for (int i = NO_OUTPUTS-1; i >= 0; i--) {
		plain_mask <<= 1;
		plain_state <<= 1;

		_flicker_enabled[i] = ((_outputs_state[i] & 0xC0) == 0x40);
		if (!_flicker_enabled[i]) {
			_flicker_counters[i] = 0;
			if (_outputs_state[i] & 0x80) { // S-COM
				scom_output(i, _outputs_state[i] & 0x7F);
			} else { // plain output
				scom_output(i, -1);
				plain_mask |= 1;
				if (_outputs_state[i] & 1)
					plain_state |= 1;
			}
		}
	}

	io_set_outputs_raw_mask(plain_state, plain_mask);
}

void outputs_set_zipped(uint8_t data[], size_t length) {
	if (length < 6)
		return;

	uint32_t full_mask = (((uint32_t)data[3]) << 24) | (((uint32_t)data[2]) << 16) | (((uint32_t)data[1]) << 8) | data[0];
	uint32_t bin_state = (((uint32_t)data[7]) << 24) | (((uint32_t)data[6]) << 16) | (((uint32_t)data[5]) << 8) | data[4];
	size_t bytei = 8;

	for (uint8_t i = 0; i < NO_OUTPUTS_ALL; i++) {
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
}

void outputs_set_full(uint8_t data[NO_OUTPUTS_ALL]) {
	memcpy((uint8_t*)_outputs_state, (uint8_t*)data, NO_OUTPUTS_ALL);
	outputs_need_apply = true;
}

void outputs_update(void) {
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
	outputs_need_apply = true;
}
