#ifndef _VARS_H_
#define _VARS_H_

/* Global variables, shared across many files
 */

#include <stdint.h>
#include <stdbool.h>

extern bool state_beacon; // beacon led active
extern bool state_nomtb;
extern bool state_readdress; // allow to change adress by broadcast
extern volatile bool mtbbus_auto_speed_in_progress;


#endif
