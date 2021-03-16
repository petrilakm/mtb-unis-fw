/* Main source file of MTB-UNI v4 CPU ATmega128.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include "io.h"

///////////////////////////////////////////////////////////////////////////////

int main();
static inline void init();

///////////////////////////////////////////////////////////////////////////////

uint16_t counter_100us = 0;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		wdt_reset();
	}
}

static inline void init() {
	WDTCR |= 1 << WDE;  // watchdog enable
	WDTCR |= WDP2; // ~250 ms timeout

	// Setup timer 0 TODO
	/*TCCR0A |= 1 << WGM01; // CTC mode
	TCCR0B |= 1 << CS01; // no prescaler
	TIMSK |= 1 << OCIE0A; // enable compare match A
	OCR0A = 99;*/

	sei(); // enable interrupts globally
}

/*ISR(TIM0_COMPA_vect) {
	// Timer 0 @ 10 kHz (period 100 us)
}*/
