#include "synctimer.h"
#include "common.h"
#include "scom.h"

unsigned long int synct_count_internal; // phase accumulator
int synct_count;
int synct_speed;
bool synct_event_pass;
bool synct_event_pre;

// list of events
synct_event_t synct_list_event[16];
// list of dividers (event frequency)
uint8_t synct_list_divider[16];

// manage events
void synct_event_add(synct_event_t evt)
{
	for(uint8_t i = 0; i < 16; i++) {
		if (synct_list_event[i] == (synct_event_t) 0) {
			synct_list_event[i] = evt;
			return;
		}
	}
}
void synct_event_remove(synct_event_t);

// helper
unsigned long int synct_abs(long int a)
{
	return (a>0) ? a : -a;
}

// init
void synct_init()
{
	synct_speed = 128;
	synct_count_internal = 0;
	synct_count = 0;

	for(uint8_t i = 0; i < 16; i++) {
		synct_list_event[i] = (synct_event_t) 0;
	}
	synct_event_pass = false;
	synct_event_pre = false;
}

// rollover over the top
void synct_reset()
{
	synct_count_internal = 0;
	synct_event_pass = false;
	synct_event_pre = false;

	
	// call all subscribed events
	/*
	for(uint8_t i = 0; i < 16; i++) {
		if (synct_list_event[i] != (synct_event_t) 0) {
			(synct_list_event[i])();
		}
	}
	*/
}

// timer ticking (each 10 ms)
void synct_tick() // call each 10 ms;
{
	synct_count_internal += synct_speed;
	
	if ((synct_count_internal > (SYNCT_WAIT)) && (!synct_event_pre)) {
		synct_event_pre = true;
		scom_sync_event_pre();
	}

	if (synct_count_internal > (SYNCT_MAX)) {
		synct_reset();
		scom_sync_event();
	}

	synct_count = (synct_count_internal >> 8);

	// check if event occur
	if ((!synct_event_pass) && (synct_count > 50)) {
		synct_event_pass = true;

	}
}

// timer sync (each 5 sec external event)
void synct_sync()
{
  // if we are in sync, then
	// synct_count_internal = 1000 * 500 = 500 000
	static int diff_last = -999;
	long int lidiff = (synct_count_internal - SYNCT_MAX);
	// regulator, ToDo: PID
	int diff = (((long int )lidiff) >> 10);
	//int corr = 0; // correction
	
	// P-part
	if (diff_last != -999) {
	  // ignore first sync, we need some statistics
		
		int diff_abs = synct_abs(diff);
		
		if (synct_abs(diff_last - diff) < 100) {
			// mala zmena od posledne, mame fazi skoro dobre
			if (diff_abs < 300) {
				(diff++);
			}
		} else {
		  // worse than last time
			// correct speed
			if (diff_abs < 50000)
			synct_speed = 1000 - (diff >> 6);
		}
	  // not in sync
		// do great change
		
		if (diff_abs)
		synct_speed += diff >> 10;
	}
	diff_last = diff;
}

