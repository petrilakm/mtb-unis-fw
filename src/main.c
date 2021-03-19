/* Main source file of MTB-UNI v4 CPU ATmega128.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include "io.h"
#include "scom.h"
#include "outputs.h"
#include "config.h"
#include "inputs.h"
#include "../lib/mtbbus.h"

///////////////////////////////////////////////////////////////////////////////

int main();
static inline void init();
void mtbbus_received(bool broadcast, uint8_t *data, uint8_t size);

///////////////////////////////////////////////////////////////////////////////

#define LED_GR_ON 5
#define LED_GR_OFF 2
volatile uint8_t led_gr_counter = 0;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		if (config_write) {
			config_save();
			config_write = false;
		}

		if (led_gr_counter > 0) {
			led_gr_counter--;
			if (led_gr_counter == LED_GR_OFF)
				io_led_green_off();
		}

		_delay_ms(10);
		// wdt_reset();
	}
}

static inline void init() {
	// WDTCR |= 1 << WDE;  // watchdog enable
	// WDTCR |= WDP2; // ~250 ms timeout

	io_init();
	io_led_red_on();
	io_led_green_on();
	io_led_blue_on();
	scom_init();

	// Setup timer 1 @ 10 kHz (period 100 us)
	TCCR1B = (1 << WGM12) | (1 << CS10); // CTC mode, no prescaler
	TIMSK |= (1 << OCIE1A); // enable compare match interrupt
	OCR1A = 1473;

	// Setup timer 3 @ 100 Hz (period 10 ms)
	TCCR3B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, 64× prescaler
	ETIMSK |= (1 << OCIE3A); // enable compare match interrupt
	OCR3A = 2302;

	config_load();
	outputs_set_full(config_safe_state);

	uint8_t _mtbbus_addr = io_get_addr_raw();
	if (_mtbbus_addr == 0)
		_mtbbus_addr = 1; // TODO: report error
	mtbbus_init(_mtbbus_addr, config_mtbbus_speed);
	mtbbus_on_receive = mtbbus_received;

	_delay_ms(50);
	sei(); // enable interrupts globally
	io_led_red_off();
	io_led_green_off();
	io_led_blue_off();
}

ISR(TIMER1_COMPA_vect) {
	// Timer 1 @ 10 kHz (period 100 us)
	inputs_debounce_update();
}

ISR(TIMER3_COMPA_vect) {
	// Timer 3 @ 100 Hz (period 10 ms)
	scom_update();
	outputs_update();
	inputs_fall_update();
}

void btn_on_pressed() {
	// TODO: blink LED (do not do it interrupt!)
	uint8_t _mtbbus_addr = io_get_addr_raw();
	if (_mtbbus_addr == 0)
		_mtbbus_addr = 1; // TODO: report error
	mtbbus_addr = _mtbbus_addr;
}

void btn_on_depressed() {}

///////////////////////////////////////////////////////////////////////////////

void mtbbus_received(bool broadcast, uint8_t *data, uint8_t size) {
	if (size < 2)
		return;

	if (led_gr_counter == 0) {
		io_led_green_on();
		led_gr_counter = LED_GR_ON;
	}

	uint8_t command_code = data[0];

	if (command_code == 0x01) {
	}
}

///////////////////////////////////////////////////////////////////////////////
