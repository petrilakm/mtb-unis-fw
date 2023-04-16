#ifndef _SERVO_H_
#define _SERVO_H_

/* manage servo operations
 */

void servo_init(void);
void servo_update(void); // call every 10 ms

void servo_set_enable(void); // propagate enabled and disabled servos to outputs

#endif