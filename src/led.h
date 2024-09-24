#ifndef _LED_H_
#define _LED_H_

/* Indication LED handling.
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "io.h"

// all durations are relative to 100 Hz timig signal
#define LED_GREEN_NOMTB_ON  (50)
#define LED_GREEN_NOMTB_OFF (25)

#define LED_RED_OK_ON 10
//#define LED_RED_OK_OFF 10
//#define LED_RED_ERR_ON 100
//#define LED_RED_ERR_OFF 50

#define LED_BLUE_BEACON_ON  (100)
#define LED_BLUE_BEACON_OFF  (50)
#define LED_BLUE_READDRESS_ON  (20)
#define LED_BLUE_READDRESS_OFF (10)
//#define LED_BLUE_OK 4

extern bool led_red_flashing;
extern bool led_state_autospeed;

void led_update(void);

void led_red_ok(void);
void led_green_ok(void);

static inline void led_green_on() { PORTG |= (1 << PIN_LED_GREEN); }
static inline void led_green_off() { PORTG &= ~(1 << PIN_LED_GREEN); }
static inline void led_red_on() { PORTG |= (1 << PIN_LED_RED); }
static inline void led_red_off() { PORTG &= ~(1 << PIN_LED_RED); }
static inline void led_blue_on() { PORTG |= (1 << PIN_LED_BLUE); }
static inline void led_blue_off() { PORTG &= ~(1 << PIN_LED_BLUE); }

/*****************************************************************************/

#endif
