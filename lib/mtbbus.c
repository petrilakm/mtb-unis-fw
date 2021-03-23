#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "mtbbus.h"
#include "../src/io.h"
#include "crc16modbus.h"

uint8_t mtbbus_output_buf[MTBBUS_OUTPUT_BUF_MAX_SIZE];
uint8_t mtbbus_output_buf_size = 0;
uint8_t mtbbus_next_byte_to_send = 0;
bool sending = false;

uint8_t mtbbus_input_buf[MTBBUS_INPUT_BUF_MAX_SIZE];
uint8_t mtbbus_input_buf_size = 0;
bool receiving = false;
volatile uint16_t received_crc = 0;
volatile uint8_t received_addr;

uint8_t mtbbus_addr;
uint8_t mtbbus_speed;
void (*mtbbus_on_receive)(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len) = NULL;
void (*mtbbus_on_sent)() = NULL;

///////////////////////////////////////////////////////////////////////////////

static void _send_next_byte();
static inline void _mtbbus_send_buf();
static inline void _mtbbus_received_ninth(uint8_t data);
static inline void _mtbbus_received_non_ninth(uint8_t data);

///////////////////////////////////////////////////////////////////////////////
// Init

void mtbbus_init(uint8_t addr, uint8_t speed) {
	mtbbus_addr = addr;

	DDRE |= 0x04; // UART direction
	uart_in();

	mtbbus_set_speed(speed);

	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 9-bit data
	UCSR0B = _BV(RXCIE0) | _BV(TXCIE0) | _BV(UCSZ02) | _BV(RXEN0) | _BV(TXEN0);  // RX, TX enable; RT, TX interrupt enable
}


void mtbbus_set_speed(uint8_t speed) {
	mtbbus_speed = speed;
	UBRR0H = 0;

	if (speed == MTBBUS_SPEED_115200)
		UBRR0L = 16;
	else if (speed == MTBBUS_SPEED_57600)
		UBRR0L = 32;
	else
		UBRR0L = 48; // 38400 Bd

	UCSR0A |= _BV(U2X0);
}

///////////////////////////////////////////////////////////////////////////////
// Sending

int mtbbus_send(uint8_t *data, uint8_t size) {
	if (!mtbbus_can_fill_output_buf())
		return 1;
	if (size > MTBBUS_OUTPUT_BUF_MAX_SIZE_USER)
		return 2;

	for (uint8_t i = 0; i < size; i++)
		mtbbus_output_buf[i] = data[i];
	mtbbus_output_buf_size = size;

	mtbbus_send_buf();
	return 0;
}

int mtbbus_send_buf() {
	if (sending)
		return 1;

	size_t i = mtbbus_output_buf_size;
	uint16_t crc = crc16modbus_bytes(0, mtbbus_output_buf, mtbbus_output_buf_size);
	mtbbus_output_buf_size += 2;
	mtbbus_output_buf[i] = crc & 0xFF;
	mtbbus_output_buf[i+1] = (crc >> 8) & 0xFF;
	_mtbbus_send_buf();
	return 0;
}

int mtbbus_send_buf_autolen() {
	if (sending)
		return 1;
	if (mtbbus_output_buf[0] > MTBBUS_OUTPUT_BUF_MAX_SIZE_USER)
		return 2;
	mtbbus_output_buf_size = mtbbus_output_buf[0]+1;
	mtbbus_send_buf();
	return 0;
}

static inline void _mtbbus_send_buf() {
	sending = true;
	mtbbus_next_byte_to_send = 0;
	uart_out();

	while (!(UCSR0A & _BV(UDRE0)));
	_send_next_byte();
}

static void _send_next_byte() {
	loop_until_bit_is_set(UCSR0A, UDRE0); // wait for empty transmit buffer
	UCSR0B &= ~_BV(TXB80); // 9 bit = 0
	UDR0 = mtbbus_output_buf[mtbbus_next_byte_to_send];
	mtbbus_next_byte_to_send++;
}

ISR(USART0_TX_vect) {
	if (mtbbus_next_byte_to_send < mtbbus_output_buf_size) {
		_send_next_byte();
	} else {
		uart_in();
		sending = false;
		if (mtbbus_on_sent != NULL) {
			void (*tmp)() = mtbbus_on_sent;
			mtbbus_on_sent = NULL;
			tmp();
		}
	}
}

bool mtbbus_can_fill_output_buf() {
	return !sending;
}

///////////////////////////////////////////////////////////////////////////////
// Receiving

ISR(USART0_RX_vect) {
	uint8_t status = UCSR0A;
	bool ninth = (UCSR0B >> 1) & 0x01;
	uint8_t data = UDR0;

	if (status & ((1<<FE0)|(1<<DOR0)|(1<<UPE0)))
		return; // return on error

	if (ninth)
		_mtbbus_received_ninth(data);
	else
		_mtbbus_received_non_ninth(data);
}

static inline void _mtbbus_received_ninth(uint8_t data) {
	received_addr = data;

	if ((received_addr == mtbbus_addr) || (received_addr == 0)) {
		receiving = true;
		mtbbus_input_buf_size = 0;
		received_crc = crc16modbus_byte(0, received_addr);
	}
}

static inline void _mtbbus_received_non_ninth(uint8_t data) {
	if (!receiving)
		return;

	if (mtbbus_input_buf_size < MTBBUS_INPUT_BUF_MAX_SIZE) {
		if ((mtbbus_input_buf_size == 0) || (mtbbus_input_buf_size <= mtbbus_input_buf[0]))
			received_crc = crc16modbus_byte(received_crc, data);
		mtbbus_input_buf[mtbbus_input_buf_size] = data;
		mtbbus_input_buf_size++;
	}

	if (mtbbus_input_buf_size >= mtbbus_input_buf[0]+3) {
		// whole message received
		uint16_t msg_crc = (mtbbus_input_buf[mtbbus_input_buf_size-1] << 8) | (mtbbus_input_buf[mtbbus_input_buf_size-2]);
		if (received_crc == msg_crc) {
			if (mtbbus_on_receive != NULL)
				mtbbus_on_receive(received_addr == 0, mtbbus_input_buf[1],
				                  mtbbus_input_buf+2, mtbbus_input_buf_size-3);
		}

		// Prepare for next receiving from XpressNET device
		receiving = false;
		received_crc = 0;
		mtbbus_input_buf_size = 0;
	}
}
