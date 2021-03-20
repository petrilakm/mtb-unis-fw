#ifndef _INPUTS_H_
#define _INPUTS_H_

/* Inputs debouncing & holding in logical one per configured time.
 */

#include <stdint.h>
#include <stdbool.h>

// This variable always contains current state of inputs which could be reported
// to master on MTBbus.
extern uint16_t inputs_logic_state;
extern uint16_t inputs_debounced_state;

extern bool btn_pressed;
void btn_on_pressed();
void btn_on_depressed();

// You may use this variable for any purpose, this unit does not work with it.
extern uint16_t inputs_old;

// This function should be called each 100 us
void inputs_debounce_update();

// This function should be called each 10 ms
void inputs_fall_update();

#endif
