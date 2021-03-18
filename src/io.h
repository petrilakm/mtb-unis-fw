#ifndef _IO_H_
#define _IO_H_

#include <stdbool.h>
#include <stdint.h>

#define NO_INPUTS 16
#define INPUT_0 PINF7
#define INPUT_1 PINF6
#define INPUT_2 PINF5
#define INPUT_3 PINF4
#define INPUT_4 PINF3
#define INPUT_5 PINF2
#define INPUT_6 PINF1
#define INPUT_7 PINF0
#define INPUT_8 PINE3
#define INPUT_9 PINE4
#define INPUT_10 PINE5
#define INPUT_11 PINE6
#define INPUT_12 PINE7
#define INPUT_13 PINB0
#define INPUT_14 PINB4
#define INPUT_15 PINB5

#define NO_OUTPUTS 16
#define OUTPUT_0 PIND7
#define OUTPUT_1 PIND6
#define OUTPUT_2 PIND5
#define OUTPUT_3 PIND4
#define OUTPUT_4 PIND3
#define OUTPUT_5 PIND2
#define OUTPUT_6 PIND1
#define OUTPUT_7 PIND0
#define OUTPUT_8 PINC7
#define OUTPUT_9 PINC6
#define OUTPUT_10 PINC5
#define OUTPUT_11 PINC4
#define OUTPUT_12 PINC3
#define OUTPUT_13 PINC2
#define OUTPUT_14 PINC1
#define OUTPUT_15 PINC0

void io_init();

bool io_get_input_raw(uint8_t inum);
uint16_t io_get_inputs_raw();

void io_set_output_raw(uint8_t onum, bool state);
void io_set_outputs_raw(uint16_t state);
void io_set_outputs_raw_mask(uint16_t state, uint16_t mask);
uint16_t io_get_outputs_raw();
bool io_get_output_raw(uint8_t onum);


#endif
