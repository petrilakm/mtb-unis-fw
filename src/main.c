/* Main source file of MTB-UNI v4 CPU's ATmega128 firmware.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>

#include "common.h"
#include "io.h"
#include "scom.h"
#include "outputs.h"
#include "config.h"
#include "inputs.h"
#include "servo.h"
#include "diag.h"
#include "../lib/mtbbus.h"
#include "../lib/crc16modbus.h"

///////////////////////////////////////////////////////////////////////////////
// Function prototypes

int main();
static void init(void);
static void init_post(void);
void mtbbus_received(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len); // intentionally not static
static void mtbbus_send_ack(void);
static void mtbbus_send_inputs(uint8_t message_code);
static void mtbbus_send_error(uint8_t code);
static inline void leds_update(void);
void goto_bootloader(void); // intentionally not static
static inline void update_mtbbus_polarity(void);
static void led_red_ok(void);
static void led_blue_ok(void);
static inline void on_initialized(void);
static inline bool mtbbus_addressed(void);
static inline void btn_short_press(void);
static inline void btn_long_press(void);
static inline void autodetect_mtbbus_speed(void);
static inline void autodetect_mtbbus_speed_stop(void);
static void mtbbus_auto_speed_next(void);
static inline void mtbbus_auto_speed_received(void);
static void send_diag_value(uint8_t i);

static void handle_flags(void);
static void timer_100hz(void);
static void handle_timers(void);


///////////////////////////////////////////////////////////////////////////////
// Defines & global variables

#define LED_GR_ON 5
#define LED_GR_OFF 2
uint8_t led_gr_counter = 0;

#define LED_RED_OK_ON 40
#define LED_RED_OK_OFF 20
#define LED_RED_ERR_ON 100
#define LED_RED_ERR_OFF 50
uint8_t led_red_counter = 0;

bool beacon = false;

#define LED_BLUE_BEACON_ON 100
#define LED_BLUE_BEACON_OFF 50
#define LED_BLUE_OK 4
uint8_t led_blue_counter = 0;

volatile bool inputs_debounce_to_update = false;
volatile bool scom_to_update = false;
bool outputs_changed_when_setting_scom = false;

__attribute__((used, section(".fwattr"))) struct {
	uint8_t no_pages;
	uint16_t crc;
} fwattr;

bool initialized = false;
uint8_t _init_counter = 0;
bool _init_counter_flag = false;
uint8_t _init_counter2 = 0;
bool _init_counter2_flag = false;
#define INIT_TIME 50 // 500 ms
#define INIT2_TIME 50 // 500 ms after

#define MTBBUS_TIMEOUT_MAX 100 // 1 s
uint8_t mtbbus_timeout = MTBBUS_TIMEOUT_MAX; // increment each 10 ms

#define BTN_PRESS_1S 100
uint8_t btn_press_time = 0;

volatile bool mtbbus_auto_speed_in_progress = false;
volatile uint8_t mtbbus_auto_speed_timer = 0;
volatile uint8_t mtbbus_auto_speed_last;
#define MTBBUS_AUTO_SPEED_TIMEOUT 20 // 200 ms

volatile uint8_t diag_timer = 0;
volatile bool t3_elapsed = false;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		mtbbus_update();
		outputs_apply_state();
		handle_flags();

		wdt_reset();
	}
}

static void handle_flags(void) {
	if (_init_counter_flag == true) {
		_init_counter_flag = false;
		on_initialized(); // set leds
	}

	if (_init_counter2_flag == true) {
		_init_counter2_flag = false;
		init_post();	// init servo positions
	}

	if (scom_to_update) {
		outputs_changed_when_setting_scom = false;
		scom_update();
		scom_to_update = false;
	}

	if (inputs_debounce_to_update) {
		inputs_debounce_to_update = false;
		inputs_debounce_update();
	}
	if (t3_elapsed) {
		t3_elapsed = false;
		timer_100hz();
	}
	if (config_write) {
		config_save();
		config_write = false;
	}
}

static void timer_100hz(void) {
	scom_to_update = true;
	servo_update();
	outputs_update();
	inputs_fall_update();
	leds_update();
	handle_timers();
}

static void handle_timers(void) {
	if (_init_counter < INIT_TIME) {
		_init_counter++;
		if (_init_counter == INIT_TIME)
			_init_counter_flag = true;
	} else {
		if (_init_counter2 < INIT2_TIME) {
			_init_counter2++;
			if (_init_counter2 == INIT2_TIME)
				_init_counter2_flag = true;
		}
	}

	if (mtbbus_timeout < MTBBUS_TIMEOUT_MAX)
		mtbbus_timeout++;

	if ((btn_pressed) && (btn_press_time < 100)) {
		btn_press_time++;
		if (btn_press_time >= 100)
			btn_long_press();
	}

	if (mtbbus_auto_speed_in_progress) {
		mtbbus_auto_speed_timer++;
		if (mtbbus_auto_speed_timer >= MTBBUS_AUTO_SPEED_TIMEOUT)
			mtbbus_auto_speed_next();
	}

	{
		static uint8_t diag_timer = 0;
		diag_timer++;
		if (diag_timer >= DIAG_UPDATE_PERIOD) {
			diag_update();
			diag_timer = 0;
		}
	}
}


void init(void) {
	cli();
	wdt_disable();
	TIMSK &= 0xC3; // clean-up after bootloader 
	ETIMSK = 0;  // ...

	// Check reset flags
	mcucsr.all = MCUCSR;
	MCUCSR = 0;

	if (config_is_int_wdrf()) {
		mcucsr.bits.wdrf = false;
		config_int_wdrf(false);
	}
	mcucsr.bits.borf = false; // brownout detects basically all power-on resets
	mtbbus_warn_flags.all = (mcucsr.all >> 1) & 0x0F;

        uart_in();
	io_init();
	io_led_red_on();
	io_led_blue_on();
	scom_init();

	// Setup timer 0 @ 100 Hz (period 10 ms)
	TCCR0  = (1 << WGM01); // CTC mode
	OCR0 = 143; // 100.00000 Hz
	TCCR0 |= 7; // 1024× prescaler, run
	TIMSK |= (1 << OCIE0) | (1 << TOIE0); // enable compare match interrupt

	// Setup timer 2 @ 2 kHz
	TCCR2 = (1 << WGM21);  // CTC mode
	OCR2  = 114; // 2003.478261 Hz
	TCCR2 |= 5;  // 64× prescaler, run
	TIMSK |= (1 << OCIE2);  // enable compare match interrupt

	// Setup timers for servo operation
	TCCR1A = 0;
	TCCR1B = (1 << WGM13); // phase & freq. corrent PWM, stop
	TCNT1 = 0;
	OCR1A = 1375;
	OCR1B = 1375;
	OCR1C = 1375;
	ICR1 = 18432;
	TIMSK |= (1 << TICIE1); // overflow on top
	TCCR1B |= 2; // 8× prescaller, run

	TCCR3A = 0;
	TCCR3B = (1 << WGM33); // phase & freq. corrent PWM, stop
	TCNT3 = 0;
	OCR3A = 1375;
	OCR3B = 1375;
	OCR3C = 1375;
	ICR3 = 18432;
	ETIMSK |= (1 << TICIE3);
	TCCR3B |= 2; // 8× prescaller, run

	config_load();
	outputs_set_full(config_safe_state);
	servo_init();

	error_flags.bits.addr_zero = (config_mtbbus_addr == 0);
	mtbbus_init(config_mtbbus_addr, config_mtbbus_speed);
	mtbbus_on_receive = mtbbus_received;

	update_mtbbus_polarity();
	diag_init();

	mtbbus_warn_flags_old.all = 0xFF; // causes report of change to PC
	wdt_enable(WDTO_250MS);
	sei(); // enable interrupts globally
}

static void init_post(void) {
	uint8_t i;
	uint8_t inp;
	for(i=0; i < NO_SERVOS; i++) {
		inp = (inputs_logic_state >> (i*2)) & 3; // extract input state
		servo_init_position(i, (inp == 2)); // pos 2 -> servopos 2, else use servopos 1
	}
	PORTB |= (1 << PB4); // servo power enable
}

static inline void soft_reset() {
	__asm__ volatile ("ijmp" ::"z" (0));
}

static inline void hard_reset() {
	wdt_enable(WDTO_15MS);
	while (true);
}

static void on_initialized(void) {
	io_led_red_off();
	io_led_green_off();
	io_led_blue_off();
	initialized = true;
}

ISR(TIMER2_COMP_vect) {
	// Timer 2 @ 2 kHz (period 500 us)
	inputs_debounce_to_update = true;
}

ISR(TIMER0_COMP_vect) {
	// Timer 0 @ 100 Hz (period 10 ms)
	if (TCNT0 > 0)
		mtbbus_warn_flags.bits.missed_timer = true;

	t3_elapsed = true;
}

ISR(TIMER1_COMPA_vect) {
	// must be empty, because bootloader start timer and ISR !
}
ISR(TIMER3_COMPA_vect) {
	// must be empty, because bootloader start timer and ISR !
}

// enable disable servo signals on demand (and in right time)
ISR(TIMER1_CAPT_vect) {
	uint8_t i;
	for (i=0; i<3; i++) {
		if ((servo_state[i] & 0x10) > 0) {
			servo_set_enable_one(i, false);
		} else {
			servo_set_enable_one(i, true);
		}
	}
}
ISR(TIMER3_CAPT_vect) {
	uint8_t i;
	for (i=3; i<6; i++) {
		if ((servo_state[i] & 0x10) > 0) {
			servo_set_enable_one(i, false);
		} else {
			servo_set_enable_one(i, true);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////

void leds_update(void) {
	if (led_gr_counter > 0) {
		led_gr_counter--;
		if (led_gr_counter == LED_GR_OFF)
			io_led_green_off();
	}

	bool led_red_flashing = error_flags.all;

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
		if (led_blue_counter == 0)
			io_led_blue_off();
	}
	if ((beacon) && (led_blue_counter == 0)) {
		led_blue_counter = LED_BLUE_BEACON_ON;
		io_led_blue_on();
	}
}

void led_red_ok(void) {
	if (led_red_counter == 0) {
		led_red_counter = LED_RED_OK_ON;
		io_led_red_on();
	}
}

void led_blue_ok(void) {
	if (led_blue_counter == 0) {
		led_blue_counter = LED_BLUE_OK;
		io_led_blue_on();
	}
}

///////////////////////////////////////////////////////////////////////////////

void btn_on_pressed(void) {
	btn_press_time = 0;
}

void btn_on_depressed(void) {
	if (btn_press_time < BTN_PRESS_1S)
		btn_short_press();
}

void btn_short_press(void) {
	if (mtbbus_auto_speed_in_progress) {
		autodetect_mtbbus_speed_stop();
		return;
	}

	if (mtbbus_addr != 0)
		led_red_ok();
	update_mtbbus_polarity();
}

void btn_long_press(void) {
	if (!mtbbus_addressed())
		autodetect_mtbbus_speed();
}

///////////////////////////////////////////////////////////////////////////////

void mtbbus_received(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len) {
	if (!initialized)
		return;

	error_flags.bits.bad_mtbbus_polarity = false;
	if (led_gr_counter == 0) {
		io_led_green_on();
		led_gr_counter = LED_GR_ON;
	}
	if ((!broadcast) && (command_code != MTBBUS_CMD_MOSI_MODULE_INQUIRY)) {
		led_blue_ok();
	}
	_delay_us(2);

	mtbbus_timeout = 0;
	if (mtbbus_auto_speed_in_progress) {
		mtbbus_auto_speed_received();
	}

	switch (command_code) {

	case MTBBUS_CMD_MOSI_MODULE_INQUIRY:
		if ((!broadcast) && (data_len >= 1)) {
			static bool last_input_changed = false;
			static bool last_diag_changed = false;
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

				if ((mtbbus_warn_flags.all != mtbbus_warn_flags_old.all) || (last_diag_changed && !last_ok)) {
					last_diag_changed = true;
					mtbbus_warn_flags_old = mtbbus_warn_flags;
					send_diag_value(MTBBUS_DV_STATE);
				} else {
					mtbbus_send_ack();
				}
			}
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_INFO_REQ:
		if (!broadcast) {
			uint16_t bootloader_ver = config_bootloader_version();
			mtbbus_output_buf[0] = 9;
			mtbbus_output_buf[1] = MTBBUS_CMD_MISO_MODULE_INFO;
			mtbbus_output_buf[2] = CONFIG_MODULE_TYPE;
			mtbbus_output_buf[3] = (mtbbus_warn_flags.all > 0) << 2;
			mtbbus_output_buf[4] = CONFIG_FW_MAJOR;
			mtbbus_output_buf[5] = CONFIG_FW_MINOR;
			mtbbus_output_buf[6] = CONFIG_PROTO_MAJOR;
			mtbbus_output_buf[7] = CONFIG_PROTO_MINOR;
			mtbbus_output_buf[8] = bootloader_ver >> 8;
			mtbbus_output_buf[9] = bootloader_ver & 0xFF;
			mtbbus_send_buf_autolen();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_SET_CONFIG:
		if ((data_len >= 56) && (!broadcast)) {
			uint8_t pos = 0;
			memcpy(config_safe_state, data, NO_OUTPUTS_ALL);
			pos = NO_OUTPUTS_ALL; // 16+12= 28
			memcpy(config_inputs_delay, data+pos, NO_OUTPUTS/2);
			pos += (NO_OUTPUTS/2); // 8 -> 36
			config_servo_enabled = data[pos];
			pos++; // 1 -> 37
			memcpy(config_servo_position, data+pos, NO_SERVOS*2);
			pos += (NO_SERVOS*2); // 12 -> 49
			for (size_t i = 0; i < NO_SERVOS; i++)
				if (data[pos+i] > 0) {
					config_servo_speed[i] = data[pos+i];
				} else {
					config_servo_speed[i] = 30;
				}
			// pos += NO_SERVOS; // 6 -> 55
			config_write = true;
			mtbbus_send_ack();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_GET_CONFIG:
		if (!broadcast) {
			uint8_t pos = 0;
			mtbbus_output_buf[0] = 56;
			mtbbus_output_buf[1] = MTBBUS_CMD_MISO_MODULE_CONFIG;
			pos = 2;
			memcpy((uint8_t*)mtbbus_output_buf+pos, config_safe_state, NO_OUTPUTS_ALL);
			pos += NO_OUTPUTS_ALL;
			memcpy((uint8_t*)mtbbus_output_buf+pos, config_inputs_delay, NO_OUTPUTS/2);
			pos += NO_OUTPUTS/2;
			mtbbus_output_buf[pos] = config_servo_enabled;
			pos += 1;
			memcpy((uint8_t*)mtbbus_output_buf+pos, config_servo_position, NO_SERVOS*2);
			pos += NO_SERVOS*2*2;
			memcpy((uint8_t*)mtbbus_output_buf+pos, config_servo_speed, NO_SERVOS);
			pos += NO_SERVOS;
			mtbbus_send_buf_autolen();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_BEACON:
		if (data_len >= 1) {
			beacon = data[0];
			if (!broadcast)
				mtbbus_send_ack();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_GET_INPUT:
		if (!broadcast) {
			mtbbus_send_inputs(MTBBUS_CMD_MISO_INPUT_STATE);
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_SET_OUTPUT:
		if ((data_len >= 6) && (!broadcast)) {
			// Send response first, because setting of outputs takes some time
			// TODO: make output_set_zipped faster?
			mtbbus_output_buf[0] = data_len+1;
			mtbbus_output_buf[1] = MTBBUS_CMD_MISO_OUTPUT_SET;
			memcpy((uint8_t*)mtbbus_output_buf+2, data, data_len);
			mtbbus_send_buf_autolen();

			outputs_set_zipped(data, data_len);
			outputs_changed_when_setting_scom = true;
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_RESET_OUTPUTS:
		if (!broadcast)
			mtbbus_send_ack();
		outputs_set_full(config_safe_state);
		break;

	// specific commands for UNIS
	case MTBBUS_CMD_MOSI_SPECIFIC:
		if (!broadcast) {
			// end of manual positioning
			if ((data_len >= 3) && (data[0] == 3) && (data[1] == 0)) {
				servo_test_select = 255;
				mtbbus_send_ack();
			// set manual position - override normal control
			} else if ((data_len >= 4) && data[0] == 3) {
				uint8_t servo_num = (data[1] >> 1) - 1;
				if (servo_num < NO_SERVOS) {
					// right servo selected
					servo_test_select = servo_num;
					servo_test_pos = data[2];
					mtbbus_send_ack();
				} else {
					mtbbus_send_error(MTBBUS_ERROR_UNKNOWN_COMMAND);
				}
			} else
				mtbbus_send_error(MTBBUS_ERROR_UNKNOWN_COMMAND);
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_CHANGE_ADDR:
		if (data_len >= 1) {
			if (broadcast) {
				if (io_button()) {
					if (data[0] > 0) {
						config_mtbbus_addr = data[0];
						config_write = true;
						mtbbus_set_addr(config_mtbbus_addr);
					}
				}
			} else {
				if (data[0] > 0) {
					config_mtbbus_addr = data[0];
					config_write = true;
					mtbbus_send_ack();
					mtbbus_set_addr(config_mtbbus_addr);
				} else {
					mtbbus_send_error(MTBBUS_ERROR_BAD_ADDRESS);
				}
			}
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_CHANGE_SPEED:
		if (data_len >= 1) {
			config_mtbbus_speed = data[0];
			config_write = true;
			mtbbus_set_speed(data[0]);

			if (!broadcast)
				mtbbus_send_ack();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_FWUPGD_REQUEST:
		if ((data_len >= 1) && (!broadcast)) {
			config_boot_fwupgd();
			mtbbus_on_sent = &goto_bootloader;
			mtbbus_send_ack();
		} else { goto INVALID_MSG; }
		break;

	case MTBBUS_CMD_MOSI_REBOOT:
		if (broadcast) {
			goto_bootloader();
		} else {
			mtbbus_on_sent = &goto_bootloader;
			mtbbus_send_ack();
		}
		break;

	case MTBBUS_CMD_MOSI_DIAG_VALUE_REQ:
		if (data_len >= 1) {
			send_diag_value(data[0]);
		} else { goto INVALID_MSG; }
		break;

INVALID_MSG:
	default:
		if (!broadcast)
			mtbbus_send_error(MTBBUS_ERROR_UNKNOWN_COMMAND);

	};
}

// Warning: functions below don't check mtbbus_can_fill_output_buf(), bacause
// they should be called ONLY from mtbbus_received event (as MTBbus is
// request-response based bus).

void mtbbus_send_ack(void) {
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

void goto_bootloader(void) {
	config_int_wdrf(true);
	hard_reset();
}

static inline void update_mtbbus_polarity(void) {
	error_flags.bits.bad_mtbbus_polarity = !((PINE >> PIN_UART_RX) & 0x1);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool mtbbus_addressed(void) {
	return mtbbus_timeout < MTBBUS_TIMEOUT_MAX;
}

///////////////////////////////////////////////////////////////////////////////

void autodetect_mtbbus_speed(void) {
	io_led_blue_on();
	mtbbus_auto_speed_in_progress = true;
	mtbbus_auto_speed_last = 0; // relies on first speed 38400 kBd = 0x01
	mtbbus_auto_speed_next();
}

void mtbbus_auto_speed_next(void) {
	mtbbus_auto_speed_timer = 0;
	mtbbus_auto_speed_last++; // relies on continuous interval of speeds
	if (mtbbus_auto_speed_last > MTBBUS_SPEED_MAX)
		mtbbus_auto_speed_last = MTBBUS_SPEED_38400;
	mtbbus_set_speed(mtbbus_auto_speed_last);
}

void mtbbus_auto_speed_received(void) {
	mtbbus_auto_speed_in_progress = false;
	config_mtbbus_speed = mtbbus_speed;
	config_write = true;
	io_led_blue_off();
}

void autodetect_mtbbus_speed_stop(void) {
	if (mtbbus_auto_speed_in_progress) {
		mtbbus_auto_speed_in_progress = false;
		io_led_blue_off();
	}
}

///////////////////////////////////////////////////////////////////////////////

void send_diag_value(uint8_t i) {
	mtbbus_output_buf[1] = MTBBUS_CMD_MISO_DIAG_VALUE;
	mtbbus_output_buf[2] = i;

	switch (i) {
	case MTBBUS_DV_VERSION:
		mtbbus_output_buf[0] = 2+1;
		mtbbus_output_buf[3] = 0x10;
		break;

	case MTBBUS_DV_STATE:
		mtbbus_output_buf[0] = 2+1;
		mtbbus_output_buf[3] = (mtbbus_warn_flags.all > 0) << 1;
		break;

	case MTBBUS_DV_UPTIME:
		mtbbus_output_buf[0] = 2+4;
		mtbbus_output_buf[3] = (uptime_seconds >> 24);
		mtbbus_output_buf[4] = (uptime_seconds >> 16) & 0xFF;
		mtbbus_output_buf[5] = (uptime_seconds >> 8) & 0xFF;
		mtbbus_output_buf[6] = (uptime_seconds) & 0xFF;
		break;

	case MTBBUS_DV_WARNINGS:
		mtbbus_warn_flags_old = mtbbus_warn_flags;
		mtbbus_output_buf[0] = 2+1;
		mtbbus_output_buf[3] = mtbbus_warn_flags.all;
		break;

	case MTBBUS_DV_VMCU:
		mtbbus_output_buf[0] = 2+2;
		mtbbus_output_buf[3] = vcc_voltage >> 8;
		mtbbus_output_buf[4] = vcc_voltage & 0xFF;
		break;

	default:
		mtbbus_output_buf[0] = 2+0;
		mtbbus_warn_flags_old = mtbbus_warn_flags;
	}

	mtbbus_send_buf_autolen();
}

///////////////////////////////////////////////////////////////////////////////
