#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/boot.h>

#include "io.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

int main();
static inline void init();
static inline void main_program();

///////////////////////////////////////////////////////////////////////////////
// Defines & global variables


///////////////////////////////////////////////////////////////////////////////

int main() {
	init();
	io_led_green_on();
	_delay_ms(500);
	io_led_green_off();
	main_program();
}

static inline void init() {
	cli();
	wdt_disable();
	io_init();
}

static inline void main_program() {
	__asm__ volatile ("ijmp" ::"z" (0));
}
