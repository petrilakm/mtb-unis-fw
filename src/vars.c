#include "vars.h"

bool state_beacon = false; // beacon led active
bool state_nomtb = false;
bool state_readdress = false; // allow to change adress by broadcast
volatile bool mtbbus_auto_speed_in_progress = false;

