#ifndef _OUTPUTS_H_
#define _OUTPUTS_H_

#include <stdint.h>
#include <stddef.h>

// Data in zipped format according to protocol's «Set Output» command description:
// https://github.com/kmzbrnoI/mtbbus-protocol/blob/master/modules/uni.md#module-specific-commands
void outputs_set(uint8_t data[], size_t length);
void outputs_update(); // should be called each 10 ms

#endif
