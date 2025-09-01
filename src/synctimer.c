#include "synctimer.h"
#include "common.h"

unsigned long int synct_count_internal;
int synct_count;
int synct_speed;

// list of events
synct_event_t synct_list_event[16];
// list of dividers (event frequency)
uint8_t synct_list_divider[16];

// manage events
void synct_event_add(synct_event_t);
void synct_event_remove(synct_event_t);

// helper
unsigned long int synct_abs(long int a)
{
	return (a>0) ? a : -a;
}

// init
void synct_init()
{
	synct_speed = 1000;
	synct_count_internal = 0;
	synct_count = 0; 
}


// timer ticking (each 10 ms)
void synct_tick() // call each 10 ms;
{
	synct_count_internal += synct_speed;
}

// timer sync (each 5 sec)
void synct_sync()
{
  // if we are in sync, then
	// synct_count_internal = 1000 * 500 = 500 000
	static int diff_last = -999;
	long int lidiff = (synct_count_internal - 500000);
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

