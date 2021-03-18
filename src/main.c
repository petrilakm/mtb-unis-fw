/* Main source file of MTB-UNI v4 CPU ATmega128.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include "io.h"
#include "scom.h"

///////////////////////////////////////////////////////////////////////////////

int main();
static inline void init();

///////////////////////////////////////////////////////////////////////////////

// uint16_t counter_100us = 0;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		_delay_ms(10);
		// wdt_reset();
	}
}

static inline void init() {
	// WDTCR |= 1 << WDE;  // watchdog enable
	// WDTCR |= WDP2; // ~250 ms timeout

	io_init();
	scom_init();

	// Setup timer 1 @ 10 kHz (period 100 us)
	TCCR1B = (1 << WGM12) | (1 << CS10); // CTC mode, no prescaler
	TIMSK |= (1 << OCIE1A); // enable compare match interrupt
	OCR1A = 1473;

	// Setup timer 3 @ 100 Hz (period 10 ms)
	TCCR3B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, 64× prescaler
	ETIMSK |= (1 << OCIE3A); // enable compare match interrupt
	OCR3A = 2302;

	sei(); // enable interrupts globally
}

ISR(TIMER1_COMPA_vect) {
	// Timer 1 @ 10 kHz (period 100 us)
}

ISR(TIMER3_COMPA_vect) {
	// Timer 3 @ 100 Hz (period 10 ms)
	scom_update();
}
