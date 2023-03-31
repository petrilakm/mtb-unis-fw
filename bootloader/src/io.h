#ifndef _IO_H_
#define _IO_H_

/* Raw input-output operation of bootloader of MTB-UNI v4 module.
 */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

#define PIN_LED_RED PG0
//#define PIN_LED_GREEN P6
#define PIN_LED_BLUE PG3

#define PIN_UART_RX PE0
#define PIN_UART_TX PE1
#define PIN_UART_DIR PE2

#define PIN_BUTTON PING2

static inline void io_init() {
	DDRD = 0; // disable outputs 0-7
	DDRC = 0; // disable outputs 8-15
	DDRB = 0xF0; // servo outputs
              DDRE = 0x38; // servo outputs
	DDRG = (1 << PIN_LED_RED) | (1 << PIN_LED_BLUE);
	PORTG = (1 << PIN_BUTTON); // button pull-up
}

static inline uint8_t io_get_addr_raw() {
	//return ~PINA;
              return 1;
}

static inline void io_led_red_on() { PORTG |= (1 << PIN_LED_RED); }
static inline void io_led_red_off() { PORTG &= ~(1 << PIN_LED_RED); }
static inline void io_led_green_on() { }
static inline void io_led_green_off() { }
static inline void io_led_blue_on() { PORTG |= (1 << PIN_LED_BLUE); }
static inline void io_led_blue_off() { PORTG &= ~(1 << PIN_LED_BLUE); }

static inline void io_led_red(bool state) {
	if (state)
		io_led_red_on();
	else
		io_led_red_off();
}

static inline void io_led_green(bool state) {
/*
	if (state)
		io_led_green_on();
	else
		io_led_green_off();
*/
}

static inline void io_led_blue(bool state) {
	if (state)
		io_led_blue_on();
	else
		io_led_blue_off();
}

static inline bool io_led_red_state() { return (PORTG >> PIN_LED_RED) & 0x1; }
//static inline bool io_led_green_state() { return (PORTB >> PIN_LED_GREEN) & 0x1; }
static inline bool io_led_blue_state() { return (PORTG >> PIN_LED_BLUE) & 0x1; }

static inline void io_led_red_toggle() { io_led_red(!io_led_red_state()); }
static inline void io_led_green_toggle() { }
static inline void io_led_blue_toggle() { io_led_blue(!io_led_blue_state()); }

static inline bool io_button() { return (PING >> PIN_BUTTON) & 0x1; }

static inline void uart_in() { PORTE |= (1 << PIN_UART_DIR); }
static inline void uart_out() { PORTE &= ~(1 << PIN_UART_DIR); }

#endif
