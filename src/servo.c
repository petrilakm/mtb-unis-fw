#include <stdbool.h>
#include "servo.h"
#include "io.h"
#include "config.h"

int32_t servo_pos[NO_SERVOS] = {0,};
uint8_t servo_state[NO_SERVOS] = {0,};

void servo_set_raw(uint8_t num, uint16_t pos) {
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

uint8_t servo_get_input_state(uint8_t num) {
	if (num > 5) {
		return 0;
	}
	uint8_t tmp;
	//num = 5-num; 
	// get only 2 bits from input vector
	tmp = ((output_virt >> ((5-num)*2)) & 0x03);
	switch (tmp) {
		case 1:
			servo_state[num] = 1;
			return 1;
		case 2:
			servo_state[num] = 0;
			return 2;
		default:
			return 0;
	}
}

uint16_t servo_get_config_position(uint8_t num, uint8_t state) {
	if (state == 1) {
		return config_servo_position[num*2+0];
	}
	if (state == 2) {
		return config_servo_position[num*2+1];
	}
	return 1375;
}

uint8_t servo_get_config_speed(uint8_t num) {
	return config_servo_speed[num];
}


void servo_init(void) {
	// timers inicialized in main
	servo_set_enable();
	PORTB |= (1 << PB4); // servo power enable
	uint8_t i;
	// for each servo
	for(i=0; i<6; i++) {
		servo_pos[i] = servo_get_config_position(i, 1) << 3;
	}
}

void servo_update(void) {
	uint8_t i;
	uint8_t state;
	uint8_t speed;
	int32_t pos_end;
	// for each servo
	for(i=0; i<6; i++) {
		servo_get_input_state(i);
		//if (state > 0) {
			state = servo_state[i];
			pos_end = servo_get_config_position(i, state) << 3;
			speed = servo_get_config_speed(i);
			int32_t diff = (servo_pos[i] - pos_end);
			int32_t absdiff = (diff > 0) ? diff : -diff;
			if ((absdiff) < speed) {
				// end position
				servo_pos[i] = pos_end;
			} else {
				if (diff > 0) {
					servo_pos[i] -= speed;
				}
				if (diff < 0) {
					servo_pos[i] += speed;
				}
			}
			servo_set_raw(i, (servo_pos[i]) >> 3);
		//}
	}
}

void servo_set_enable(void) {
	uint8_t mask = config_servo_enabled;

	if (mask & (1 << 0)) { // servo 1
		TCCR1A |=  (1 << COM1C0);
		TCCR1A |=  (1 << COM1C1);
	} else {
		TCCR1A &= ~(1 << COM1C0);
		TCCR1A &= ~(1 << COM1C1);
	}
	if (mask & (1 << 1)) { // servo 2
		TCCR1A |=  (1 << COM1B0);
		TCCR1A |=  (1 << COM1B1);
	} else {
		TCCR1A &= ~(1 << COM1B0);
		TCCR1A &= ~(1 << COM1B1);
	}
	if (mask & (1 << 2)) { // servo 3
		TCCR1A |=  (1 << COM1A0);
		TCCR1A |=  (1 << COM1A1);
	} else {
		TCCR1A &= ~(1 << COM1A0);
		TCCR1A &= ~(1 << COM1A1);
	}
	if (mask & (1 << 3)) { // servo 4
		TCCR3A |=  (1 << COM3C0);
		TCCR3A |=  (1 << COM3C1);
	} else {
		TCCR3A &= ~(1 << COM3C0);
		TCCR3A &= ~(1 << COM3C1);
	}
	if (mask & (1 << 4)) { // servo 5
		TCCR3A |=  (1 << COM3B0);
		TCCR3A |=  (1 << COM3B1);
	} else {
		TCCR3A &= ~(1 << COM3B0);
		TCCR3A &= ~(1 << COM3B1);
	}
	if (mask & (1 << 5)) { // servo 6
		TCCR3A |=  (1 << COM3A0);
		TCCR3A |=  (1 << COM3A1);
	} else {
		TCCR3A &= ~(1 << COM3A0);
		TCCR3A &= ~(1 << COM3A1);
	}
}