#ifndef _DIAG_H_
#define _DIAG_H_

/* Diagnostics */

#include <stdbool.h>
#include <stdint.h>

typedef union {
	struct {
		bool porf : 1;
		bool extrf : 1;
		bool borf : 1;
		bool wdrf : 1;
		bool jtrf : 1;
	} bits;
	uint8_t all;
} mcucsr_t;

extern mcucsr_t mcucsr;


typedef union {
	struct {
		bool addr_zero : 1;
		bool bad_mtbbus_polarity : 1;
	} bits;
	uint8_t all;
} error_flags_t;

extern error_flags_t error_flags;


typedef union {
	struct {
		bool extrf : 1;
		bool borf : 1;
		bool wdrf : 1;
		bool jtrf : 1;
		bool missed_timer : 1;
		bool vcc_oscilating : 1;
	} bits;
	uint8_t all;
} mtbbus_warn_flags_t;

extern mtbbus_warn_flags_t mtbbus_warn_flags;
extern mtbbus_warn_flags_t mtbbus_warn_flags_old;

void diag_init(void);
void diag_update(void); // called each 100 ms
void vcc_start_measure(void);

#define DIAG_UPDATE_PERIOD 10 // 100 ms
extern volatile uint16_t vcc_voltage;
#define VCC_MAX_DIFF 10 // 0.2 V

extern volatile uint16_t mcu_temperature;
extern uint8_t ts_offset;
extern uint8_t ts_gain;
extern volatile uint32_t uptime_seconds;

#endif
