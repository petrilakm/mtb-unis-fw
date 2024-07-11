#include "pwmsw.h"

const uint8_t pwm_tableon[16] PROGMEM = { // PWM table off -> on
	10,
	50,
	25,
	16,
	12,
	10,
	5,
	100,
	50,
    10,
    20,
    30,
    40,
    50,
    60,
    70,
};

const uint8_t pwm_tableoff[16] PROGMEM = { // PWM table on -> off
	255,
	200,
	170,
	160,
	150,
	140,
	130,
	120,
	110,
    100,
    90,
    80,
    70,
    60,
    50,
    40,
};
