/*
  Synchronized timer and counter
	
	synchronization via MTB-BUS ovel all devices
	
	base speed = 20 Hz
	timer period = 5 s
	
	2025, Michal Petrilak
*/

#ifndef _SYNCTIMER_H_
#define _SYNCTIMER_H_

#include "io.h"
#include "config.h"

typedef void (*synct_event_t)(void);

extern int synct_count;
void synct_event_add(synct_event_t);
void synct_event_remove(synct_event_t);
void synct_init();
void synct_tick(); // call each 10 ms;
void synct_sync(); // bus-sync


#endif
