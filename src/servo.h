#ifndef _SERVO_H_
#define _SERVO_H_

#include "io.h"
#include "config.h"

/* manage servo operations
 */

#define SERVO_TIMEOUT_MAX (150)

extern volatile uint8_t servo_state[NO_SERVOS];

void servo_init(void);
void servo_update(void); // call every 10 ms

void servo_set_enable(void); // propagate enabled and disabled servos to outputs
void servo_set_enable_one(uint8_t servo, bool state); // propagate enabled and disabled servo to one output

#endif