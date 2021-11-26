/* Main source file of MTB-UNI v4 CPU's ATmega128 firmware.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include "common.h"
#include "io.h"
#include "scom.h"
#include "outputs.h"
#include "config.h"
#include "inputs.h"
#include "../lib/mtbbus.h"
#include "../lib/crc16modbus.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

int main();
static inline void init();
void mtbbus_received(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len);
void mtbbus_send_ack();
void mtbbus_send_inputs(uint8_t message_code);
void mtbbus_send_error(uint8_t code);
static inline void leds_update();
void goto_bootloader();
static inline void update_mtbbus_polarity();
void led_red_ok();
static inline void on_initialized();

///////////////////////////////////////////////////////////////////////////////
// Defines & global variables

typedef union {
	struct {
		bool porf : 1;
		bool extrf : 1;
		bool borf : 1;
		bool wdrf : 1;
		bool jtrf : 1;
	} bits;
	uint8_t all;
} mcuscr_t;

mcuscr_t mcucsr;


#define BEACON_LED_RED // code for MTB-UNI v4.0 should define this variable

#define LED_GR_ON 5
#define LED_GR_OFF 2
volatile uint8_t led_gr_counter = 0;

#define LED_RED_OK_ON 40
#define LED_RED_OK_OFF 20
#define LED_RED_ERR_ON 100
#define LED_RED_ERR_OFF 50
volatile uint8_t led_red_counter = 0;

typedef union {
	struct {
		bool addr_zero : 1;
		bool bad_mtbbus_polarity : 1;
		bool missed_timer : 1;
		bool bad_reset : 1;
	} bits;
	uint8_t all;
} error_flags_t;

error_flags_t error_flags = {0};

volatile bool beacon = false;

#define LED_BLUE_BEACON_ON 100
#define LED_BLUE_BEACON_OFF 50
volatile uint8_t led_blue_counter = 0;

volatile bool inputs_debounce_to_update = false;
volatile bool scom_to_update = false;
volatile bool outputs_changed_when_setting_scom = false;

__attribute__((used, section(".fwattr"))) struct {
	uint8_t no_pages;
	uint16_t crc;
} fwattr;

volatile bool initialized = false;
volatile uint8_t _init_counter = 0;
#define INIT_TIME 50 // 500 ms

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		if (scom_to_update) {
			outputs_changed_when_setting_scom = false;
			scom_update();
			scom_to_update = false;
			if (outputs_changed_when_setting_scom)
				outputs_apply_state();
		}
		if (inputs_debounce_to_update) {
			inputs_debounce_update();
			inputs_debounce_to_update = false;
		}

		if (config_write) {
			config_save();
			config_write = false;
		}

		wdt_reset();
		_delay_us(50);
	}
}

static inline void init() {
	cli();
	wdt_disable();

	// Check reset flags
	mcucsr.all = MCUCSR;
	MCUCSR = 0;

	if (config_is_int_wdrf()) {
		mcucsr.bits.wdrf = false;
		config_int_wdrf(false);
	}
	mcucsr.bits.borf = false; // brownout detects basically all power-on resets
	if (mcucsr.all > 1)
		error_flags.bits.bad_reset = true;

	io_init();
	io_led_red_on();
	io_led_green_on();
	io_led_blue_on();
	scom_init();

	// Setup timer 1 @ 10 kHz (period 100 us)
	TCCR1B = (1 << WGM12) | (1 << CS10); // CTC mode, no prescaler
	TIMSK = (1 << OCIE1A); // enable compare match interrupt
	OCR1A = 7365;

	// Setup timer 3 @ 100 Hz (period 10 ms)
	TCCR3B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, 64× prescaler
	ETIMSK = (1 << OCIE3A); // enable compare match interrupt
	OCR3A = 2302;

	config_load();
	outputs_set_full(config_safe_state);

	uint8_t _mtbbus_addr = io_get_addr_raw();
	error_flags.bits.addr_zero = (_mtbbus_addr == 0);
	mtbbus_init(_mtbbus_addr, config_mtbbus_speed);
	mtbbus_on_receive = mtbbus_received;

	update_mtbbus_polarity();

	wdt_enable(WDTO_250MS);
	sei(); // enable interrupts globally
}

static inline void on_initialized() {
	io_led_red_off();
	io_led_green_off();
	io_led_blue_off();
	initialized = true;
}

ISR(TIMER1_COMPA_vect) {
	// Timer 1 @ 2 kHz (period 500 us)
	inputs_debounce_to_update = true;
}

ISR(TIMER3_COMPA_vect) {
	// Timer 3 @ 100 Hz (period 10 ms)
	if ((TCNT1H > 0) & (TCNT1H < OCR1AH))
		error_flags.bits.missed_timer = true;

	scom_to_update = true;
	outputs_update();
	inputs_fall_update();
	leds_update();

	if (_init_counter < INIT_TIME) {
		_init_counter++;
		if (_init_counter == INIT_TIME)
			on_initialized();
	}
}

///////////////////////////////////////////////////////////////////////////////

static inline void leds_update() {
	if (led_gr_counter > 0) {
		led_gr_counter--;
		if (led_gr_counter == LED_GR_OFF)
			io_led_green_off();
	}
	_delay_us(2);

	bool led_red_flashing = error_flags.all;
	#ifdef BEACON_LED_RED
	led_red_flashing |= beacon;
	#endif

	if (led_red_counter > 0) {
		led_red_counter--;
		if (((!led_red_flashing) && (led_red_counter == LED_RED_OK_OFF)) ||
			((led_red_flashing) && (led_red_counter == LED_RED_ERR_OFF)))
			io_led_red_off();
	}
	if ((led_red_flashing) && (led_red_counter == 0)) {
		led_red_counter = LED_RED_ERR_ON;
		io_led_red_on();
	}

	if (led_blue_counter > 0) {
		led_blue_counter--;
		if (led_blue_counter == LED_BLUE_BEACON_OFF)
			io_led_blue_off();
	}
	if ((beacon) && (led_blue_counter == 0)) {
		led_blue_counter = LED_BLUE_BEACON_ON;
		io_led_blue_on();
	}
}

void led_red_ok() {
	if (led_red_counter == 0) {
		led_red_counter = LED_RED_OK_ON;
		io_led_red_on();
	}
}

///////////////////////////////////////////////////////////////////////////////

void btn_on_pressed() {
	uint8_t _mtbbus_addr = io_get_addr_raw();
	error_flags.bits.addr_zero = (_mtbbus_addr == 0);
	mtbbus_addr = _mtbbus_addr;
	if (mtbbus_addr != 0)
		led_red_ok();
	update_mtbbus_polarity();
}

void btn_on_depressed() {}

///////////////////////////////////////////////////////////////////////////////

void mtbbus_received(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len) {
	if (!initialized)
		return;

	error_flags.bits.bad_mtbbus_polarity = false;
	if (led_gr_counter == 0) {
		io_led_green_on();
		led_gr_counter = LED_GR_ON;
	}

	if ((!broadcast) && (command_code == MTBBUS_CMD_MOSI_MODULE_INQUIRY) && (data_len >= 1)) {
		static bool last_input_changed = false;
		static bool first_scan = true;
		bool last_ok = data[0] & 0x01;
		if ((inputs_logic_state != inputs_old) || (last_input_changed && !last_ok) || (first_scan)) {
			// Send inputs changed
			last_input_changed = true;
			first_scan = false;
			mtbbus_send_inputs(MTBBUS_CMD_MISO_INPUT_CHANGED);
			inputs_old = inputs_logic_state;
		} else {
			last_input_changed = false;
			mtbbus_send_ack();
		}

	} else if ((command_code == MTBBUS_CMD_MOSI_INFO_REQ) && (!broadcast)) {
		mtbbus_output_buf[0] = 7;
		mtbbus_output_buf[1] = MTBBUS_CMD_MISO_MODULE_INFO;
		mtbbus_output_buf[2] = CONFIG_MODULE_TYPE;
		mtbbus_output_buf[3] = 0x00; // module flags
		mtbbus_output_buf[4] = CONFIG_FW_MAJOR;
		mtbbus_output_buf[5] = CONFIG_FW_MINOR;
		mtbbus_output_buf[6] = CONFIG_PROTO_MAJOR;
		mtbbus_output_buf[7] = CONFIG_PROTO_MINOR;
		mtbbus_send_buf_autolen();

	} else if ((command_code == MTBBUS_CMD_MOSI_SET_CONFIG) && (data_len >= 24) && (!broadcast)) {
		for (size_t i = 0; i < NO_OUTPUTS; i++)
			config_safe_state[i] = data[i];
		for (size_t i = 0; i < NO_OUTPUTS/2; i++)
			config_inputs_delay[i] = data[NO_OUTPUTS+i];
		config_write = true;
		mtbbus_send_ack();

	} else if ((command_code == MTBBUS_CMD_MOSI_GET_CONFIG) && (!broadcast)) {
		mtbbus_output_buf[0] = 25;
		mtbbus_output_buf[1] = MTBBUS_CMD_MISO_MODULE_CONFIG;
		for (size_t i = 0; i < NO_OUTPUTS; i++)
			mtbbus_output_buf[2+i] = config_safe_state[i];
		for (size_t i = 0; i < NO_OUTPUTS/2; i++)
			mtbbus_output_buf[2+NO_OUTPUTS+i] = config_inputs_delay[i];
		mtbbus_send_buf_autolen();

	} else if ((command_code == MTBBUS_CMD_MOSI_BEACON) && (data_len >= 1)) {
		beacon = data[0];
		if (!broadcast)
			mtbbus_send_ack();

	} else if ((command_code == MTBBUS_CMD_MOSI_GET_INPUT) && (!broadcast)) {
		mtbbus_send_inputs(MTBBUS_CMD_MISO_INPUT_STATE);

	} else if ((command_code == MTBBUS_CMD_MOSI_SET_OUTPUT) && (data_len >= 4) && (!broadcast)) {
		outputs_set_zipped(data, data_len);
		outputs_changed_when_setting_scom = true;

		mtbbus_output_buf[0] = data_len+1;
		mtbbus_output_buf[1] = MTBBUS_CMD_MISO_OUTPUT_SET;
		for (size_t i = 0; i < data_len; i++)
			mtbbus_output_buf[2+i] = data[i];
		mtbbus_send_buf_autolen();

	} else if (command_code == MTBBUS_CMD_MOSI_RESET_OUTPUTS) {
		outputs_set_full(config_safe_state);

		if (!broadcast)
			mtbbus_send_ack();

	} else if ((command_code == MTBBUS_CMD_MOSI_CHANGE_ADDR) && (data_len >= 1) && (!broadcast)) {
		mtbbus_send_error(MTBBUS_ERROR_UNSUPPORTED_COMMAND);

	} else if ((command_code == MTBBUS_CMD_MOSI_CHANGE_SPEED) && (data_len >= 1)) {
		config_mtbbus_speed = data[0];
		config_write = true;
		mtbbus_set_speed(data[0]);

		if (!broadcast)
			mtbbus_send_ack();

	} else if ((command_code == MTBBUS_CMD_MOSI_FWUPGD_REQUEST) && (data_len >= 1) && (!broadcast)) {
		config_boot_fwupgd();
		mtbbus_on_sent = &goto_bootloader;
		mtbbus_send_ack();

	} else if (command_code == MTBBUS_CMD_MOSI_REBOOT) {
		if (broadcast) {
			goto_bootloader();
		} else {
			mtbbus_on_sent = &goto_bootloader;
			mtbbus_send_ack();
		}

	} else {
		if (!broadcast)
			mtbbus_send_error(MTBBUS_ERROR_UNKNOWN_COMMAND);
	}
}

// Warning: functions below don't check mtbbus_can_fill_output_buf(), bacause
// they should be called ONLY from mtbbus_received event (as MTBbus is
// request-response based bus).

void mtbbus_send_ack() {
	mtbbus_output_buf[0] = 1;
	mtbbus_output_buf[1] = MTBBUS_CMD_MISO_ACK;
	mtbbus_send_buf_autolen();
}

void mtbbus_send_inputs(uint8_t message_code) {
	mtbbus_output_buf[0] = 3;
	mtbbus_output_buf[1] = message_code;
	mtbbus_output_buf[2] = (inputs_logic_state >> 8) & 0xFF;
	mtbbus_output_buf[3] = inputs_logic_state & 0xFF;
	mtbbus_send_buf_autolen();
}

void mtbbus_send_error(uint8_t code) {
	mtbbus_output_buf[0] = 2;
	mtbbus_output_buf[1] = MTBBUS_CMD_MISO_ERROR;
	mtbbus_output_buf[2] = code;
	mtbbus_send_buf_autolen();
}

///////////////////////////////////////////////////////////////////////////////

void goto_bootloader() {
	config_int_wdrf(true);
	wdt_enable(WDTO_15MS);
	while (true);
	//__asm__ volatile ("ijmp" ::"z" (BOOTLOADER_ADDR));
}

static inline void update_mtbbus_polarity() {
	error_flags.bits.bad_mtbbus_polarity = !((PINE >> PIN_UART_RX) & 0x1);
}
