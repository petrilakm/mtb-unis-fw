#include <string.h>
#include "scom.h"
// DEBUG
#include "led.h"

int8_t _codes[NO_OUTPUTS]; // -1 = not coding anything
int8_t _codes_new[NO_OUTPUTS];
uint8_t _phase = 0;
bool scom_sync = false;  // "sync" request
bool scom_sync2= false;  // synchronized "sync" data frame transmit

#define SCOM_PHASE_STARTBIT1 (0)
#define SCOM_PHASE_STARTBIT2 (1)
#define SCOM_PHASE_BIT0      (2)
#define SCOM_PHASE_BIT1      (3)
#define SCOM_PHASE_BIT2      (4)
#define SCOM_PHASE_BIT3      (5)
#define SCOM_PHASE_BIT4      (6)
#define SCOM_PHASE_BIT5      (7)
#define SCOM_PHASE_BIT6      (8)
#define SCOM_PHASE_STOPBIT   (9)
#define SCOM_PHASE_END      (200)

void scom_init(void) {
	scom_reset();
	synct_event_add(scom_sync_event);
}

void scom_update(void) {
	uint16_t outputs = 0;
	uint16_t mask = 0;

	if (_phase == SCOM_PHASE_STARTBIT2) {
		scom_sync2 = scom_sync; // synchro sync variable
		scom_sync = false;
	}

	if (_phase <= SCOM_PHASE_STOPBIT+1) {
		for (int i = NO_OUTPUTS-1; i >= 0; i--) {
			outputs <<= 1;
			mask <<= 1;

			if (_codes[i] > -1)
				mask |= 1;

			if (_phase == SCOM_PHASE_STARTBIT1 || _phase == SCOM_PHASE_STOPBIT) {
				outputs |= 1;
			}else if ((_phase == SCOM_PHASE_BIT5) && (scom_sync2)) {
				outputs |= 1;
			} else if ((_phase >= SCOM_PHASE_BIT0) && (_phase <= SCOM_PHASE_BIT6) && (!scom_sync2)) {
				uint8_t biti = _phase - SCOM_PHASE_BIT0;
				if (((_codes[i] >> biti) & 0x1) == 0) {
					outputs |= 1;
				}
			}
		}

		io_set_outputs_raw_mask(outputs, mask);
	}

	_phase++;
	if (_phase >= SCOM_PHASE_END) {
		_phase = 0;
		memcpy((int8_t*)_codes, (int8_t*)_codes_new, NO_OUTPUTS);
		scom_sync2 = false;
	}
}

void scom_reset(void) {
	memset((int8_t*)_codes, -1, NO_OUTPUTS);
	memset((int8_t*)_codes_new, -1, NO_OUTPUTS);
}

void scom_output(uint8_t output, int8_t code) {
	if (output >= NO_OUTPUTS) {
		return;
	}
	if (_codes[output] >= 0 && code == -1) {
		_codes[output] = -1; // disable immediately
	}
	_codes_new[output] = code; // enabling | settings different code -> buffer
}

void scom_disable_output(uint8_t output) {
	if (output >= NO_OUTPUTS) {
		return;
	}
	scom_output(output, -1);
}

void scom_sync_event(void) {
	scom_sync = true;
	led_red_ok();
}

inline bool scom_is_output(uint8_t output) {
	if (output >= NO_OUTPUTS)
		return false;
	return _codes[output] > -1;
}
