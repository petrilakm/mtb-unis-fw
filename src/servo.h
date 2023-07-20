#ifndef _SERVO_H_
#define _SERVO_H_

#include "io.h"
#include "config.h"

/* manage servo operations
 */

// 1s of servo signal in sterady state
#define SERVO_TIMEOUT_MAX (50*1)
// 120s of manual positioning mode
#define SERVO_TEST_TIMEOUT_MAX (50*120)

#define SERVO_OFFSET_POS (46)

extern volatile uint8_t servo_state[NO_SERVOS];
extern uint16_t servo_pos[NO_SERVOS];
extern volatile uint8_t servo_test_select;
extern volatile uint16_t servo_test_pos;

void servo_init(void);
void servo_init_position(uint8_t servo, bool state); // inis servo last position
void servo_update(void); // call every 10 ms

void servo_set_enable(void); // propagate enabled and disabled servos to outputs
void servo_set_enable_one(uint8_t servo, bool state); // propagate enabled and disabled servo to one output

#endif
