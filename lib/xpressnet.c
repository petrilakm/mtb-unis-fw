#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define BAUD 62500
#include <util/setbaud.h>

#include "xpressnet.h"
#include "../src/hardware.h"

uint8_t uart_output_buf[UART_OUTPUT_BUF_MAX_SIZE];
uint8_t uart_output_buf_size = 0;
uint8_t uart_next_byte_to_send = 0;
bool sending = false;
bool waiting_for_send = false;
bool uart_device_addressed = false;

uint16_t uart_addressed_counter = 0;
#define UART_ADDRESSED_TIMEOUT 1000 // 1 s

uint8_t uart_input_buf[UART_INPUT_BUF_MAX_SIZE];
uint8_t uart_input_buf_size = 0;
bool receiving = false;
uint8_t received_xor = 0;
uint8_t received_addr;

uint8_t xpressnet_addr;
void (*uart_on_receive)(uint8_t recipient, uint8_t data[], uint8_t size) = NULL;
void (*uart_on_addressed)() = NULL;
void (*uart_on_addressed_stopped)() = NULL;
void (*uart_on_addr_changed)(uint8_t new_addr) = NULL;
void (*uart_on_sniff)(uint8_t sender, uint8_t *data, uint8_t size) = NULL;

///////////////////////////////////////////////////////////////////////////////

void send_next_byte();
void _uart_send_buf();
static inline void _uart_received_ninth(uint8_t data);
static inline void _uart_received_non_ninth(uint8_t data);
static inline bool _parity_ok(uint8_t data);
static inline uint8_t _message_len(uint8_t header_byte);
static inline void _check_addr_conflict();
static inline uint8_t _xor(uint8_t data[], uint8_t len);

///////////////////////////////////////////////////////////////////////////////
// Init

void uart_init(uint8_t xn_addr) {
	xpressnet_addr = xn_addr;

	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif

	// Set RS485 direction bits
	DDRD |= _BV(PORTD2); // output
	uart_in();

	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // 9-bit data
	UCSR0B = _BV(RXCIE0) | _BV(TXCIE0) | _BV(UCSZ02) | _BV(RXEN0) | _BV(TXEN0);  // RX, TX enable; RT, TX interrupt enable
}

void uart_update() {
	if (uart_addressed_counter < UART_ADDRESSED_TIMEOUT) {
		uart_addressed_counter++;

		if ((uart_addressed_counter == UART_ADDRESSED_TIMEOUT) && (uart_device_addressed)) {
			uart_device_addressed = false;
			if (uart_on_addressed_stopped != NULL)
				uart_on_addressed_stopped();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Sending

int uart_send(uint8_t *data, uint8_t size) {
	if (!uart_can_fill_output_buf())
		return 1;
	if (size > UART_OUTPUT_BUF_MAX_SIZE)
		return 2;

	for (uint8_t i = 0; i < size; i++)
		uart_output_buf[i] = data[i];
	uart_output_buf_size = size;

	uart_send_buf();
	return 0;
}

int uart_send_buf() {
	if (sending)
		return 1;
	waiting_for_send = true;
	uart_output_buf[uart_output_buf_size-1] = _xor(uart_output_buf, uart_output_buf_size-1);
	return 0;
}

int  uart_send_buf_autolen() {
	if (sending)
		return 1;
	uart_output_buf_size = _message_len(uart_output_buf[0]);
	uart_send_buf();
	return 0;
}

void _uart_send_buf() {
	sending = true;
	waiting_for_send = false;
	uart_next_byte_to_send = 0;
	uart_out();

	while (!(UCSR0A & (1<<UDRE0)));
	send_next_byte();
}

void send_next_byte() {
	loop_until_bit_is_set(UCSR0A, UDRE0); // wait for mepty transmit buffer
	UCSR0B &= ~_BV(TXB80);
	UDR0 = uart_output_buf[uart_next_byte_to_send];
	uart_next_byte_to_send++;
}

ISR(USART_TX_vect) {
	if (uart_next_byte_to_send < uart_output_buf_size) {
		send_next_byte();
	} else {
		uart_in();
		sending = false;
	}
}

bool uart_can_fill_output_buf() {
	return !sending && !waiting_for_send && uart_device_addressed;
}

///////////////////////////////////////////////////////////////////////////////
// Receiving

ISR(USART_RX_vect) {
	uint8_t status = UCSR0A;
	bool ninth = (UCSR0B >> 1) & 0x01;
	uint8_t data = UDR0;

	if (status & ((1<<FE0)|(1<<DOR0)|(1<<UPE0)))
		return; // return on error

	if (ninth)
		_uart_received_ninth(data);
	else
		_uart_received_non_ninth(data);
}

static inline void _uart_received_ninth(uint8_t data) {
	if (!_parity_ok(data))
		return;

	received_addr = data & 0x1F;

	if ((received_addr == xpressnet_addr) && (((data >> 5) & 0x03) == 0x02)) {
		// normal inquiry -> send data ASAP
		if (waiting_for_send)
			_uart_send_buf();

		uart_addressed_counter = 0;
		if (!uart_device_addressed) {
			uart_device_addressed = true;
			if (uart_on_addressed != NULL)
				uart_on_addressed();
		}
	} else if ((received_addr == xpressnet_addr) && (((data >> 5) & 0x03) == 0)) {
		// request acknowledgement
		uart_output_buf[0] = 0x20;
		uart_output_buf[1] = 0x20;
		uart_output_buf_size = 2;
		_uart_send_buf();
	} else if (((data >> 5) & 0x03) == 0x03) {
		receiving = true;
		received_xor = 0;
		uart_input_buf_size = 0;
	}
}

static inline void _uart_received_non_ninth(uint8_t data) {
	if (!receiving) {
		// Another XN device sends data
		_check_addr_conflict();
	}

	// Receive data if 'receiving' is false too -> sniff data sentt from XN
	// device to Command Station (these data start with ninth bit 0).
	// No need for timeouts, ninth bit will always reset receiving.

	if (uart_input_buf_size < UART_INPUT_BUF_MAX_SIZE) {
		received_xor ^= data;
		uart_input_buf[uart_input_buf_size] = data;
		uart_input_buf_size++;
	}

	if (uart_input_buf_size >= _message_len(uart_input_buf[0])) {
		// whole message received
		if (received_xor == 0) {
			if (receiving) {
				if (uart_on_receive != NULL)
					uart_on_receive(received_addr, uart_input_buf, uart_input_buf_size);
			} else {
				if (uart_on_sniff != NULL)
					uart_on_sniff(received_addr, uart_input_buf, uart_input_buf_size);
			}
		}

		// Prepare for next receiving from XpressNET device
		receiving = false;
		received_xor = 0;
		uart_input_buf_size = 0;
	}
}

static inline bool _parity_ok(uint8_t data) {
	bool parity = false;
	for (uint8_t i = 0; i < 8; i++) {
		parity ^= data & 0x01;
		data >>= 1;
	}
	return !parity;
}

static inline uint8_t _message_len(uint8_t header_byte) {
	return (header_byte & 0x0F) + 2;
}

static inline void _check_addr_conflict() {
	// received_addr contains last address of any incoming data
	if (received_addr == xpressnet_addr) {
		// Change XN addr +- randomly
		xpressnet_addr = (TCNT0 % XN_MAX_ADDR) + 1;
		if (uart_on_addr_changed != NULL)
			uart_on_addr_changed(xpressnet_addr);
	}
}

static inline uint8_t _xor(uint8_t data[], uint8_t len) {
	uint8_t xor = 0;
	for (uint8_t i = 0; i < len; i++)
		xor ^= data[i];
	return xor;
}
