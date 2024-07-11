#ifndef _PWMSW_H_
#define _PWMSW_H_

/* Software PWM generator
 */

#include <avr/pgmspace.h>
#include <stdbool.h>
#include "io.h"

uint8_t pwmnum[16]; // 0 - real out off, 1-16 pwm signal steps
uint8_t pwmstate[16]; // 0 - off, 1 - on
bool pwmenable[16]; // false = off


#endif
