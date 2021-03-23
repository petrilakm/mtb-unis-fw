#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>

#include "io.h"
#include "../lib/crc16modbus.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

int main();
static inline void init();
static inline void main_program();
bool fwcrc_ok(); // TODO: static inline

///////////////////////////////////////////////////////////////////////////////
// Defines & global variables

__attribute__((used, section(".fwattr"))) struct {
	uint8_t no_pages;
	uint16_t crc;
} fwattr;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();
	/*io_led_green_on();
	_delay_ms(500);
	io_led_green_off();
	main_program();*/

	fwcrc_ok();
	if (fwcrc_ok())
		io_led_green_on();
	else
		io_led_red_on();
	while (true) {}

	return 0;
}

static inline void init() {
	cli();
	wdt_disable();
	io_init();
	eeprom_busy_wait();
	boot_spm_busy_wait();
}

static inline void main_program() {
	__asm__ volatile ("ijmp" ::"z" (0));
}

///////////////////////////////////////////////////////////////////////////////

bool fwcrc_ok() {
	uint8_t no_pages = pgm_read_byte_far(&fwattr.no_pages);
	uint16_t crc_read = pgm_read_word_far(&fwattr.crc);

	uint16_t crc = 0;
	for (size_t i = 0; i < no_pages; i++)
		for (size_t j = 0; j < SPM_PAGESIZE; j++)
			crc = crc16modbus_byte(crc, pgm_read_byte_far((SPM_PAGESIZE*i) + j));

	return crc_read == crc;
}
