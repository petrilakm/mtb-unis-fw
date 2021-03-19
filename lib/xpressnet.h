#ifndef _XPRESSNET_H_
#define _XPRESSNET_H_

/* XpressNET communication lirary (via UART).
 *
 * Automatic XpressNET device address collision detection & changing relies
 * on running timer0.
 *
 * Before changing uart_output_buf & calling uart_send*, you must check that
 * uart_can_fill_output_buf() is true.
 *
 * This library uses constant adressing timeout 1 s, which is not enough for
 * Command Station in programming mode -> it will call error event
 * uart_on_addressed_stopped 1 s after service mode enter. After resuming to
 * normal mode, library will detect it and call uart_on_addressed.
 *
 * This library allows to sniff data from command station to other XpressNET
 * devices (uart_on_receive) as well as sniffing data sent from other XpressNET
 * devices to the Command Station (uart_on_sniff). If you don't want to use
 * these features, just don't register events.
 */

#include <stdio.h>
#include <stdbool.h>

#define UART_OUTPUT_BUF_MAX_SIZE 32
#define UART_INPUT_BUF_MAX_SIZE 32
extern uint8_t uart_output_buf[UART_OUTPUT_BUF_MAX_SIZE];
extern uint8_t uart_output_buf_size;

extern uint8_t uart_input_buf[UART_INPUT_BUF_MAX_SIZE];
extern uint8_t uart_input_buf_size;

#define XN_MAX_ADDR 31
extern uint8_t xpressnet_addr;

extern bool uart_device_addressed;

extern void (*uart_on_receive)(uint8_t recipient, uint8_t *data, uint8_t size);
extern void (*uart_on_sniff)(uint8_t sender, uint8_t *data, uint8_t size);
extern void (*uart_on_addressed)();
extern void (*uart_on_addressed_stopped)();
extern void (*uart_on_addr_changed)(uint8_t new_addr);

void uart_init(uint8_t xn_addr);

bool uart_can_fill_output_buf();
int uart_send(uint8_t *data, uint8_t size);
int uart_send_buf_autolen();
int uart_send_buf();
void uart_update();

#endif
