#include <stdbool.h>
#include "servo.h"
#include "inputs.h"
#include "outputs.h"

uint16_t servo_pos[NO_SERVOS] = {4000,}; // actual servo position
// last position
// state   - 1 -> no pulse on output
// 0lls 00pp
// 0?00 0010 - servo moving to position 2
// 0001 0001 - servo stopped in position 1
volatile uint8_t servo_state_target[NO_SERVOS] = {0,}; // used in ISR !
volatile uint8_t servo_state_current[NO_SERVOS] = {0,}; // used in ISR !
uint8_t servo_running[NO_SERVOS]; // bool if servo is in motion
// timeout in stop state
uint8_t servo_timeout[NO_SERVOS] = {0,};
uint8_t servo_enabled = 0;
// virtual inputs
uint16_t servos_inputs_state; // inputs from MTB point of view (virtual/real, mapped)
uint16_t servos_inputs_state_old;
// for manual servo positioning
volatile uint8_t servo_test_select = 255;
uint8_t servo_test_select_last = 255;
volatile uint16_t servo_test_pos = 0;
uint16_t servo_test_pos_last = 0;

uint16_t servo_test_timeout = 0;

void servo_set_raw(uint8_t num, uint16_t pos) {
	pos = pos >> 2;
	switch (num) {
		case 0:
			OCR1C = pos;
			break;
		case 1:
			OCR1B = pos;
			break;
		case 2:
			OCR1A = pos;
			break;
		case 3:
			OCR3C = pos;
			break;
		case 4:
			OCR3B = pos;
			break;
		case 5:
			OCR3A = pos;
			break;
	}
}

// read virtual outputs and determine servo position
uint8_t servo_get_output_state(uint8_t num) {
	if (num > NO_SERVOS) {
		return 0;
	}
	uint8_t tmp;
	// get only 2 bits from input vector
	tmp  = _outputs_state[16+num*2+0] << 0; // pos A output
	tmp |= _outputs_state[16+num*2+1] << 1; // pos B output
	//tmp = ((servos_outputs_state >> (num*2)) & 0x03);
	switch (tmp) {
		case 1:
			if ((servo_state_target[num] & 3) != 1) {
				servo_state_target[num] = 1;
				servo_timeout[num] = 0; // reset timeout
			}
			return 1;
		case 2:
			if ((servo_state_target[num] & 3) != 2) {
				servo_state_target[num] = 2;
				servo_timeout[num] = 0; // reset timeout
			}
			return 2;
		default:
			// set null state
			// servo_state[num] = 0;
			return 0;
	}
	// save last position for change detection
	servo_state_target[num] &= ~(  3 << 5);
	servo_state_target[num] |=  (tmp << 5);
	return 0;
}

// get virtual inputs for one serfo from real inputs
uint8_t servo_get_mapped_bool(uint8_t num) {
	uint8_t first_input_num = config_servo_input_map[num];
	if (first_input_num > 0) {
		return true; // mapped input, return true
	} else {
		return false; // no mapped input
	}
}

// get virtual inputs for one servo from real inputs
uint8_t servo_get_mapped_input(uint8_t num) {
	uint8_t first_input_num = config_servo_input_map[num];
	// check valid range 1-15 (1 means real inputs 0,1; 15 means real inputs 14,15)
	if ((first_input_num > 0) && (first_input_num < NO_INPUTS)) {
		first_input_num--; // real inputs count from 1 (0 means no map)
		return (inputs_logic_state >> first_input_num) & 3;
	} else {
		return 0; // no mapped input
	}
}

// read real inputs and map to virtual inputs
void servo_set_input_state(uint8_t num) {
	if (num > NO_SERVOS) {
		return;
	}
	uint8_t inpstate = 0;
	if (servo_get_mapped_bool(num)) {
		// mapped input - use real inputs by map
		inpstate = servo_get_mapped_input(num);
	} else {
		// no map - use simulated inputs
		// for now only show requested state
		if (!servo_running[num]) {
			// in end position report current position
			inpstate = (servo_state_current[num] & 3);
		}
	}

	servos_inputs_state &= ~(3 << (num*2)); // mask old state
	servos_inputs_state |=  (inpstate << (num*2)); // write current state
}

// internal function, no error checking
uint16_t servo_get_config_position_internal(uint8_t num, uint8_t internal_state) {
	return (config_servo_position[num*2+internal_state]+SERVO_OFFSET_POS) << 5;
}

// get saved position or center position
uint16_t servo_get_config_position(uint8_t num, uint8_t state) {
	if (state == 1) {
		return servo_get_config_position_internal(num, 0);
	}
	if (state == 2) {
		return servo_get_config_position_internal(num, 1);
	}
	// unknown, get center position -> calculate it
	uint16_t tmp = 0;
	tmp += (servo_get_config_position_internal(num, 0)) >> 1; // position divided by 2
	tmp += (servo_get_config_position_internal(num, 1)) >> 1; // position divided by 2
	return (tmp); // return average value avg = (a/2 + b/2) = (a+b)/2;
}

// ...
uint8_t servo_get_config_speed(uint8_t num) {
	return config_servo_speed[num];
}

// initialize servo pulse outputs, after boot
void servo_init(void) {
	// timers inicialized in main
	PORTB &= ~(1 << PB4); // servo power disable
	PORTE |= (1 << PE3); // default output for servo is L
	PORTE |= (1 << PE4);
	PORTE |= (1 << PE5);
	PORTB |= (1 << PE5);
	PORTB |= (1 << PE6);
	PORTB |= (1 << PE7);
	servo_enabled = 0; // all disable, first determine last position, then enable
}

// full initialize one servo, depend on configuration
void servo_init_position(uint8_t servo) {
	if (servo > NO_SERVOS) return;
	bool servoena;
	uint8_t statenum;
	servoena = (((config_servo_enabled >> servo) & 1) > 0);
	if (servoena) {
		if (servo_get_mapped_bool(servo)) {
			statenum = servo_get_mapped_input(servo);
		} else {
			statenum = 0; // unknown position
		}
		// determine initial servo position
		servo_state_target[servo] = statenum;
		servo_state_current[servo] = statenum;
		// load position to RAM
		servo_pos[servo] = servo_get_config_position(servo, statenum);
		// generate signal now
		servo_timeout[servo] = 100; // pulses after init => 2 s
		// enable servo
		servo_enabled |= (1 << servo);
		servo_set_raw(servo, servo_pos[servo]);
	} else {
		// disable unused servo
		servo_state_target[servo] |= 0x10;
		servo_state_current[servo] |= 0x10;
	}
}

void servo_update(void) {
	static uint8_t servo_cnt = 0;

	servo_get_output_state(i); // get requested position (servo_input <- MTB output)
	servo_set_input_state(i); // return current position (servo_output -> MTB input)

	servo_cnt++; // cycle all servos
	if (servo_cnt >= NO_SERVOS) servo_cnt = 0;

	// on deselect manual servo
	// return manual positioned servo to right location
	if (servo_test_select_last != servo_test_select) {
		servo_state_target[servo_test_select_last] &= ~16;  // enable servo signal
		servo_timeout[servo_test_select_last] = 0; // reset timeout for servo operation
		servo_test_timeout = SERVO_TEST_TIMEOUT_MAX;  // reset timeout for manual positioning end
		if (servo_test_select < NO_SERVOS) {
			servo_state_target[servo_test_select] &= ~16;  // enable servo signal for new servo
			servo_timeout[servo_test_select] = 0; // reset timeout for servo operation
		}
		servo_test_select_last = servo_test_select;
	}
	// measure timeout of manual positioning end
	if (servo_test_timeout > 0) {
		servo_test_timeout--;
		if (servo_test_timeout == 0) {
			// end of manual mode, return to normal mode
			servo_state_target[servo_test_select] &= ~16;  // enable servo signal, servo must return to defined position
			servo_timeout[servo_test_select] = 50; // reset timeout for servo operation
			servo_test_select = 255; // deselect manual servo
		}
	}

    // short name for actual servo number
	const uint8_t s = servo_cnt;
	
	// only enabled servos
	if (((servo_enabled >> s) & 1) > 0) {
		// get current servo state
		const uint8_t state = servo_state_target[s];
		// variable for end position
		uint16_t pos_end;

		if (i == servo_test_select) {
			// servo in manual mode:
			//set current position (controled in interupt via received commands)
			pos_end = (servo_test_pos+SERVO_OFFSET_POS) << 5; // set manual position

			// if manual position changed
			if (servo_test_pos != servo_test_pos_last) {
				servo_test_pos_last = servo_test_pos; // save last command
				servo_state_target[s] &= ~0x10;  // enable servo signal
				servo_timeout[s] = 0; // reset timeout for servo operation
				servo_test_timeout = SERVO_TEST_TIMEOUT_MAX;  // reset timeout for manual positioning end
			}
		} else {
			// normal operation
			//load position from config eeprom based on actual state
			pos_end = servo_get_config_position(s, state & 0x03);
		}

		// measure timeout for one servo signal
		if (servo_timeout[s] > 0) {
			servo_timeout[s]--;
			if (servo_timeout[s] == 0) {
				servo_state_target[s] |= 0x10; // timeout elapsed, stop servo signal
			}
		}

		// if servo is in moving state
		if (((state & 0x10) == 0) && (servo_timeout[s] == 0)) {
			// get configured speed
			uint8_t speed = servo_get_config_speed(s);
			// compute remaining position error
			int16_t diff = (servo_pos[s] - pos_end);
			int16_t absdiff = (diff > 0) ? diff : -diff;
			// we are far or near ?
			if ((absdiff) < speed) {
				// near - end position
				servo_pos[s] = pos_end; // move any remaining angle
				servo_timeout[s] = SERVO_TIMEOUT_MAX;
				servo_running[s] = false;
				servo_state_current[s] = servo_state_target[s]; // propagate position change to simulate end-switches
			} else {
				// far - use right speed
				if (diff > 0) {
					servo_pos[s] -= speed; // change current angle
				}
				if (diff < 0) {
					servo_pos[s] += speed; // change current angle
				}
				servo_running[s] = true;
			}
			servo_set_raw(s, servo_pos[s]);
		}
	} else {
		servo_state_target[s] |= 16; // disable unused servo
	}
}

void servo_set_enable_one(uint8_t servo, bool state)
{
	switch (servo) {
		case 0:
			if (state) { // servo 1
				TCCR1A |=  (1 << COM1C0);
				TCCR1A |=  (1 << COM1C1);
			} else {
				TCCR1A &= ~(1 << COM1C0);
				TCCR1A &= ~(1 << COM1C1);
				PORTB |= (1 << PE7);
			}
			break;
		case 1:
			if (state) { // servo 2
				TCCR1A |=  (1 << COM1B0);
				TCCR1A |=  (1 << COM1B1);
			} else {
				TCCR1A &= ~(1 << COM1B0);
				TCCR1A &= ~(1 << COM1B1);
				PORTB |= (1 << PE6);
			}
			break;
		case 2:
			if (state) { // servo 3
				TCCR1A |=  (1 << COM1A0);
				TCCR1A |=  (1 << COM1A1);
			} else {
				TCCR1A &= ~(1 << COM1A0);
				TCCR1A &= ~(1 << COM1A1);
				PORTB |= (1 << PE5);
			}
			break;
		case 3:
			if (state) { // servo 4
				TCCR3A |=  (1 << COM3C0);
				TCCR3A |=  (1 << COM3C1);
			} else {
				TCCR3A &= ~(1 << COM3C0);
				TCCR3A &= ~(1 << COM3C1);
				PORTE |= (1 << PE5);
			}
			break;
		case 4:
			if (state) { // servo 5
				TCCR3A |=  (1 << COM3B0);
				TCCR3A |=  (1 << COM3B1);
			} else {
				TCCR3A &= ~(1 << COM3B0);
				TCCR3A &= ~(1 << COM3B1);
				PORTE |= (1 << PE4);
			}
			break;
		case 5:
			if (state) { // servo 6
				TCCR3A |=  (1 << COM3A0);
				TCCR3A |=  (1 << COM3A1);
			} else {
				TCCR3A &= ~(1 << COM3A0);
				TCCR3A &= ~(1 << COM3A1);
				PORTE |= (1 << PE3);
			}
			break;
		default:
			return;
	}
}
