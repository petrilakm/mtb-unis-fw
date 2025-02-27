#ifndef _IO_H_
#define _IO_H_

/* Raw input-output operation of MTB-UNI v4 module.
 * This unit does not perform any debouncing nor any complicated IO stuff.
 * It just abstracts IO with nice & fast functions.
 */

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

extern volatile uint8_t dbg;

// Defines below are informative only, they are basically never used.
// If you change IO pins mapping, look into ‹io.c› and change approprite
// functions.

#define NO_INPUTS 16

#define NO_OUTPUTS (16)
#define NO_SERVOS (6)
#define NO_SERVO_OUTPUTS (NO_SERVOS*2)
#define NO_OUTPUTS_ALL (NO_OUTPUTS+NO_SERVO_OUTPUTS)

#define PIN_LED_GREEN PG0
#define PIN_LED_RED PG3
#define PIN_LED_BLUE PG4

#define PIN_UART_RX PE0
#define PIN_UART_TX PE1
#define PIN_UART_DIR PE2

#define PIN_BUTTON PING2

void io_init();

bool io_get_input_raw(uint8_t inum);
uint16_t io_get_inputs_raw();

void io_set_output_raw(uint8_t onum, bool state);
void io_set_outputs_raw();
void io_set_outputs_raw_mask(uint16_t state, uint16_t mask);
uint16_t io_get_outputs_raw(void);
bool io_get_output_raw(uint8_t onum);

extern volatile uint16_t output_shadow;
extern uint16_t output_virt;
extern uint8_t output_analog[8];

//static inline bool io_led_red_state() { return !((PORTG >> PIN_LED_RED) & 0x1); }
//static inline bool io_led_green_state() { return 0; }
//static inline bool io_led_blue_state() { return !((PORTG >> PIN_LED_BLUE) & 0x1); }

//static inline void io_led_red_toggle(void) { io_led_red(!io_led_red_state()); }
//static inline void io_led_green_toggle(void) { io_led_green(!io_led_green_state()); }
//static inline void io_led_blue_toggle(void) { io_led_blue(!io_led_blue_state()); }

static inline bool io_button(void) { return (PING >> PIN_BUTTON) & 0x1; }

static inline void uart_out() { PORTE &= ~(1 << PIN_UART_DIR); }
static inline void uart_in() { PORTE |= (1 << PIN_UART_DIR); }

static inline void io_tpmiso(bool state) {
	if (state)
		PORTB |= (1 << PIN_TPMISO);
	else
		PORTB &= ~(1 << PIN_TPMISO);
}

static inline void io_tpmiso_toggle(void) {
	if (PORTB & (1 << PIN_TPMISO))
		PORTB &= ~(1 << PIN_TPMISO);
	else
		PORTB |= (1 << PIN_TPMISO);
}

static inline void io_tpmosi(bool state) {
	if (state)
		PORTB |= (1 << PIN_TPMOSI);
	else
		PORTB &= ~(1 << PIN_TPMOSI);
}

static inline void io_tpmosi_toggle(void) {
	if (PORTB & (1 << PIN_TPMOSI))
		PORTB &= ~(1 << PIN_TPMOSI);
	else
		PORTB |= (1 << PIN_TPMOSI);
}

#endif
