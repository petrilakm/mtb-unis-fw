#ifndef _SCOM_H_
#define _SCOM_H_

/* S-COM signal generator
 */

#include <stdbool.h>
#include "io.h"
#include "synctimer.h"

void scom_init(void);
void scom_update(void);

void scom_reset(void);
void scom_output(uint8_t output, int8_t code);
void scom_disable_output(uint8_t output);

void scom_sync_event(void);
//inline bool scom_is_output(uint8_t output);

#endif
