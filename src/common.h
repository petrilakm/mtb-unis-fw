#ifndef _COMMON_H_
#define _COMMON_H_

/* Common functions for all units.
 */

#define BOOTLOADER_ADDR 0xF000

static inline uint8_t bit_reverse(uint8_t x) {
	x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
	x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
	x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
	return x;
}

#endif
