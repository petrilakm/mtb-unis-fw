#include <stdbool.h>
#include "servo.h"

uint16_t servo_pos[NO_SERVOS] = {4000,}; // actual servo position
// state + 4bit=no pulse state 000s 00pp
// 0000 0010 - servo end in position 2
// 0001 0001 - servo stopped in position 1
volatile uint8_t servo_state[NO_SERVOS] = {0,}; // used in ISR !
// timeout in stop state
uint8_t servo_timeout[NO_SERVOS] = {0,};
uint8_t servo_enabled = 0;
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

// read virtual inputs and determine servo position
uint8_t servo_get_input_state(uint8_t num) {
	if (num > NO_SERVOS) {
		return 0;
	}
	uint8_t tmp;
	// get only 2 bits from input vector
	tmp = ((output_virt >> (num*2)) & 0x03);
	switch (tmp) {
		case 1:
			if ((servo_state[num] & 3) != 1) {
				servo_state[num] = 1;
				servo_timeout[num] = 0; // reset timeout
			}
			return 1;
		case 2:
			if ((servo_state[num] & 3) != 2) {
				servo_state[num] = 2;
				servo_timeout[num] = 0; // reset timeout
			}
			return 2;
		default:
			// set null state
			servo_state[num] = 0;
			return 0;
	}
}

uint16_t servo_get_config_position(uint8_t num, uint8_t state) {
	if (state == 1) {
		return (config_servo_position[num*2+0]+SERVO_OFFSET_POS) << 5;
	}
	if (state == 2) {
		return (config_servo_position[num*2+1]+SERVO_OFFSET_POS) << 5;
	}
	return (127<<5);
}

uint8_t servo_get_config_speed(uint8_t num) {
	return config_servo_speed[num];
}


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

void servo_init_position(uint8_t servo, bool state) {
	if (servo > NO_SERVOS) return;
	bool servoena;
	uint8_t statenum;
	servoena = (((config_servo_enabled >> servo) & 1) > 0);
	if (servoena) {
		// determine initial servo position
		statenum = (state) ? 2 : 1;
		servo_state[servo] = statenum;
		// load position to RAM
		servo_pos[servo] = servo_get_config_position(servo, statenum);
		// generate signal now
		servo_timeout[servo] = 100; // pulses after init => 2 s
		// enable servo
		servo_enabled |= (1 << servo);
		servo_set_raw(servo, servo_pos[servo]);
	} else {
		// disable unused servo
		servo_state[servo] |= 0x10;
	}
}

void servo_update(void) {
	static uint8_t postdiv2 = 2;
	static uint8_t servo_cnt = 0;
	uint8_t i;
	uint8_t state = 0;
	uint8_t speed = 0;
	uint16_t pos_end = (100<<5);

	postdiv2--;
	if (postdiv2 == 0) {
		postdiv2 = 2;
		// 50 Hz

		servo_get_input_state(servo_cnt);

		servo_cnt++;
		if (servo_cnt>5) servo_cnt=0;

		// on deselect manual servo
		// return manual positioned servo to right location
		if (servo_test_select_last != servo_test_select) {
			servo_state[servo_test_select_last] &= ~16;  // enable servo signal
			servo_timeout[servo_test_select_last] = 0; // reset timeout for servo operation
			servo_test_timeout = SERVO_TEST_TIMEOUT_MAX;  // reset timeout for manual positioning end
			if (servo_test_select < NO_SERVOS) {
				servo_state[servo_test_select] &= ~16;  // enable servo signal for new servo
				servo_timeout[servo_test_select] = 0; // reset timeout for servo operation
			}
			servo_test_select_last = servo_test_select;
		}
		// measure timeout of manual positioning end
		if (servo_test_timeout > 0) {
			servo_test_timeout--;
			if (servo_test_timeout == 0) {
				// end of manual mode, return to normal mode
				servo_state[servo_test_select] &= ~16;  // enable servo signal
				servo_timeout[servo_test_select] = 50; // reset timeout for servo operation
				servo_test_select = 255; // deselect manual servo
			}
		}

		// for each servo
		for(i=0; i<NO_SERVOS; i++) {
			// only enabled servos
			if (((servo_enabled >> i) & 1) > 0) {
				state = servo_state[i];
				if (i == servo_test_select) {
					// servo in manual mode:
					//set current position (controled in interupt via received commands)
					pos_end = (servo_test_pos+SERVO_OFFSET_POS) << 5; // set manual position

					// if manual position changed
					if (servo_test_pos != servo_test_pos_last) {
						servo_test_pos_last = servo_test_pos; // save last command
						servo_state[i] &= ~0x10;  // enable servo signal
						servo_timeout[i] = 0; // reset timeout for servo operation
						servo_test_timeout = SERVO_TEST_TIMEOUT_MAX;  // reset timeout for manual positioning end
					}
				} else {
					// normal operation
					pos_end = servo_get_config_position(i, state & 0x03);
				}
				//state = servo_state[i];

				// measure timeout for one servo signal
				if (servo_timeout[i] > 0) {
					servo_timeout[i]--;
					if (servo_timeout[i] == 0) {
						servo_state[i] |= 0x10;
					}
				}

				if ((state < 8) && (servo_timeout[i] == 0)) {
					speed = servo_get_config_speed(i);
					int16_t diff = ( servo_pos[i] - pos_end);
					int16_t absdiff = (diff > 0) ? diff : -diff;
					if ((absdiff) < speed) {
						// end position
						servo_pos[i] = pos_end;
						servo_timeout[i] = SERVO_TIMEOUT_MAX;
					} else {
						if (diff > 0) {
							servo_pos[i] -= speed;
						}
						if (diff < 0) {
							servo_pos[i] += speed;
						}
					}
					servo_set_raw(i, servo_pos[i]);
				}
			} else {
				servo_state[i] |= 16; // disable unused servo
			}
		}
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
