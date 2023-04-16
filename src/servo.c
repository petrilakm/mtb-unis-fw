#include <stdbool.h>
#include "servo.h"
#include "io.h"
#include "config.h"

void servo_init(void) {
  // timers inicialized in main
  servo_set_enable();
}

void servo_update(void) {
  //
}

void servo_set_enable(void) {
    uint8_t mask = config_servo_enabled;

	if (mask & (1 << 0))  // servo 1
		TCCR1A |=  (1 << COM1C1);
	else
		TCCR1A &= ~(1 << COM1C1);
	if (mask & (1 << 1))  // servo 2
		TCCR1A |=  (1 << COM1B1);
	else
		TCCR1A &= ~(1 << COM1B1);
	if (mask & (1 << 2))  // servo 3
		TCCR1A |=  (1 << COM1A1);
	else
		TCCR1A &= ~(1 << COM1A1);
	if (mask & (1 << 3))  // servo 4
		TCCR3A |=  (1 << COM3C1);
	else
		TCCR3A &= ~(1 << COM3C1);
	if (mask & (1 << 4))  // servo 5
		TCCR3A |=  (1 << COM3B1);
	else
		TCCR3A &= ~(1 << COM3B1);
	if (mask & (1 << 5))  // servo 6
		TCCR3A |=  (1 << COM3A1);
	else
		TCCR3A &= ~(1 << COM3A1);
}