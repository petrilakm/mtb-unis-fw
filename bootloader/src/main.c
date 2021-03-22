#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "io.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

int main();
static inline void init();

///////////////////////////////////////////////////////////////////////////////
// Defines & global variables


///////////////////////////////////////////////////////////////////////////////

int main() {
	init();
	return 0;
}

static inline void init() {
	io_init();
	io_led_green_on();
	while (true) {
		io_led_green_toggle();
		_delay_ms(200);
	}
}
