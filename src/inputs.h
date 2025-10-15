#ifndef _INPUTS_H_
#define _INPUTS_H_

/* Inputs debouncing & holding in logical one for configured time.
 */

#include <stdint.h>
#include <stdbool.h>

// This variable always contains current state of inputs which could be reported
// to master on MTBbus.
extern uint16_t inputs_logic_state;
extern uint16_t inputs_debounced_state;

extern bool btn_pressed; // button state after debounce
extern bool io_button_short_pressed; // turn high after short press, handle in main program
extern bool io_button_long_pressed; // turn high after long press, handle in main program

// You may use this variable for any purpose, this unit does not work with it.
extern uint16_t inputs_old;

// This function should be called each 100 us
void inputs_debounce_update(void);

// These functions should be called each 10 ms
void inputs_fall_update(void);
void button_long_press_detect_update(void);

#endif
