#ifndef _IO_H_
#define _IO_H_

/* Raw input-output operation of bootloader of MTB-UNI v4 module.
 */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

#define INPUT_BUTTON PING4

static inline void io_init() {
	DDRD = 0; // disable outputs 7-0
	DDRC = 0; // disable outputs 15-8
	DDRB |= 0xC0; // LEDs PB6 (red), PB7 (green)
	DDRG |= 0x08; // LED PG3 (blue)
	PORTG |= 0x10; // button pull-up
}

static inline uint8_t io_get_addr_raw() {
	return ~PINA;
}

static inline void io_led_red_on() { PORTB |= (1 << PB7); }
static inline void io_led_red_off() { PORTB &= ~(1 << PB7); }
static inline void io_led_green_on() { PORTB |= (1 << PB6); }
static inline void io_led_green_off() { PORTB &= ~(1 << PB6); }
static inline void io_led_blue_on() { PORTG |= (1 << PG3); }
static inline void io_led_blue_off() { PORTG &= ~(1 << PG3); }

static inline void io_led_red(bool state) {
	if (state)
		io_led_red_on();
	else
		io_led_red_off();
}

static inline void io_led_green(bool state) {
	if (state)
		io_led_green_on();
	else
		io_led_green_off();
}

static inline void io_led_blue(bool state) {
	if (state)
		io_led_blue_on();
	else
		io_led_blue_off();
}

static inline bool io_led_red_state() { return (PORTB >> PB7) & 0x1; }
static inline bool io_led_green_state() { return (PORTB >> PB6) & 0x1; }
static inline bool io_led_blue_state() { return (PORTG >> PG3) & 0x1; }

static inline void io_led_red_toggle() { io_led_red(!io_led_red_state()); }
static inline void io_led_green_toggle() { io_led_green(!io_led_green_state()); }
static inline void io_led_blue_toggle() { io_led_blue(!io_led_blue_state()); }

static inline bool io_button() { return (PING >> 4) & 0x1; }

static inline void uart_out() { PORTE |= (1 << PE2); }
static inline void uart_in() { PORTE &= ~(1 << PE2); }

#endif
