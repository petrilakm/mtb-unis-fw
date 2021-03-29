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
extern void (*mtbbus_on_receive)(bool broadcast, uint8_t command_code, uint8_t *data, uint8_t data_len);
extern void (*mtbbus_on_sent)();

#define MTBBUS_SPEED_38400 0x01
#define MTBBUS_SPEED_57600 0x02
#define MTBBUS_SPEED_115200 0x03

void mtbbus_init(uint8_t addr, uint8_t speed);
void mtbbus_set_speed(uint8_t speed);

bool mtbbus_can_fill_output_buf();
int mtbbus_send(uint8_t *data, uint8_t size);
int mtbbus_send_buf_autolen();
int mtbbus_send_buf();

#define MTBBUS_CMD_MOSI_MODULE_INQUIRY 0x01
#define MTBBUS_CMD_MOSI_INFO_REQ 0x02
#define MTBBUS_CMD_MOSI_SET_CONFIG 0x03
#define MTBBUS_CMD_MOSI_GET_CONFIG 0x04
#define MTBBUS_CMD_MOSI_BEACON 0x05
#define MTBBUS_CMD_MOSI_GET_INPUT 0x10
#define MTBBUS_CMD_MOSI_SET_OUTPUT 0x11
#define MTBBUS_CMD_MOSI_RESET_OUTPUTS 0x12
#define MTBBUS_CMD_MOSI_CHANGE_ADDR 0x20
#define MTBBUS_CMD_MOSI_CHANGE_SPEED 0xE0
#define MTBBUS_CMD_MOSI_FWUPGD_REQUEST 0xF0
#define MTBBUS_CMD_MOSI_WRITE_FLASH 0xF1
#define MTBBUS_CMD_MOSI_WRITE_FLASH_STATUS_REQ 0xF2
#define MTBBUS_CMD_MOSI_SPECIFIC 0xFE
#define MTBBUS_CMD_MOSI_REBOOT 0xFF

#define MTBBUS_CMD_MISO_ACK 0x01
#define MTBBUS_CMD_MISO_ERROR 0x02
#define MTBBUS_CMD_MISO_MODULE_INFO 0x03
#define MTBBUS_CMD_MISO_MODULE_CONFIG 0x04
#define MTBBUS_CMD_MISO_INPUT_CHANGED 0x10
#define MTBBUS_CMD_MISO_INPUT_STATE 0x11
#define MTBBUS_CMD_MISO_OUTPUT_SET 0x12
#define MTBBUS_CMD_MISO_WRITE_FLASH_STATUS 0xF2
#define MTBBUS_CMD_MISO_SPECIFIC 0xFE

#define MTBBUS_ERROR_UNKNOWN_COMMAND 0x01
#define MTBBUS_ERROR_UNSUPPORTED_COMMAND 0x02
#define MTBBUS_ERROR_BAD_ADDRESS 0x03
#define MTBBUS_ERROR_BUSY 0x04

#endif
