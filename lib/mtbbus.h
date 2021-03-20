#ifndef _MTBBUS_H_
#define _MTBBUS_H_

/* MTBbus communication library (via USART).
 *
 * Before changing uart_output_buf & calling mtbbus_send*, you must check that
 * mtbbus_can_fill_output_buf() is true.
 */

#include <stdio.h>
#include <stdbool.h>

#define MTBBUS_OUTPUT_BUF_MAX_SIZE_USER 120
#define MTBBUS_OUTPUT_BUF_MAX_SIZE 128
#define MTBBUS_INPUT_BUF_MAX_SIZE 128
extern uint8_t mtbbus_output_buf[MTBBUS_OUTPUT_BUF_MAX_SIZE];
extern uint8_t mtbbus_output_buf_size;

extern uint8_t mtbbus_input_buf[MTBBUS_INPUT_BUF_MAX_SIZE];
extern uint8_t mtbbus_input_buf_size;

extern uint8_t mtbbus_addr;
extern uint8_t mtbbus_speed;

// ‹data› starts with Command code byte
// ‹size› is amount of data bytes + 1
extern void (*mtbbus_on_receive)(bool broadcast, uint8_t *data, uint8_t size);

#define MTBBUS_SPEED_38400 0x01
#define MTBBUS_SPEED_57600 0x02
#define MTBBUS_SPEED_115200 0x03

void mtbbus_init(uint8_t addr, uint8_t speed);
void mtbbus_set_speed(uint8_t speed);

bool mtbbus_can_fill_output_buf();
int mtbbus_send(uint8_t *data, uint8_t size);
int mtbbus_send_buf_autolen();
int mtbbus_send_buf();

#endif
