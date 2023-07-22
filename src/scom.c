#include <string.h>
#include "scom.h"

volatile int8_t _codes[NO_OUTPUTS]; // -1 = not coding anything
volatile int8_t _codes_new[NO_OUTPUTS];
volatile uint8_t _phase = 0;

#define SCOM_PHASE_STARTBIT1 0
#define SCOM_PHASE_STARTBIT0 1
#define SCOM_PHASE_BIT0      2
#define SCOM_PHASE_BIT6      8
#define SCOM_PHASE_STOPBIT   9
#define SCOM_PHASE_END      30

void scom_init() {
	scom_reset();
}

void scom_update() {
	uint16_t outputs = 0;
	uint16_t mask = 0;

	if (_phase <= SCOM_PHASE_STOPBIT+1) {
		for (int i = NO_OUTPUTS-1; i >= 0; i--) {
			outputs <<= 1;
			mask <<= 1;

			if (_codes[i] > -1)
				mask |= 1;

			if (_phase == SCOM_PHASE_STARTBIT1 || _phase == SCOM_PHASE_STOPBIT) {
				outputs |= 1;
			} else if (_phase >= SCOM_PHASE_BIT0 && _phase <= SCOM_PHASE_BIT6) {
				uint8_t biti = _phase - SCOM_PHASE_BIT0;
				if (((_codes[i] >> biti) & 0x1) == 0)
					outputs |= 1;
			}
		}

		io_set_outputs_raw_mask(outputs, mask);
	}

	_phase++;
	if (_phase >= SCOM_PHASE_END) {
		_phase = 0;
		memcpy((int8_t*)_codes, (int8_t*)_codes_new, NO_OUTPUTS);
	}
}

void scom_reset() {
	memset((int8_t*)_codes, -1, NO_OUTPUTS);
	memset((int8_t*)_codes_new, -1, NO_OUTPUTS);
}

void scom_output(uint8_t output, int8_t code) {
	if (output >= NO_OUTPUTS)
		return;
	if (_codes[output] >= 0 && code == -1)
		_codes[output] = -1; // disable immediately
	_codes_new[output] = code; // enabling | settings different code -> buffer
}

void scom_disable_output(uint8_t output) {
	if (output >= NO_OUTPUTS)
		return;
	scom_output(output, -1);
}

inline bool scom_is_output(uint8_t output) {
	if (output >= NO_OUTPUTS)
		return false;
	return _codes[output] > -1;
}
