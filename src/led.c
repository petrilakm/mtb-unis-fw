#include "led.h"
#include "vars.h"

uint8_t led_gr_counter = 0;
uint8_t led_red_counter = 0;
uint8_t led_blue_counter = 0;
uint8_t led_green_counter_max = 0;
uint8_t led_green_counter_mid = 0;
uint8_t led_blue_counter_max = 0;
uint8_t led_blue_counter_mid = 0;
bool led_red_flashing = false;
bool led_state_autospeed = false;

/*****************************************************************************/
// 100 Hz
void led_update(void) {
  if (state_nomtb) {
    led_green_counter_max = LED_GREEN_NOMTB_ON;
    led_green_counter_mid = LED_GREEN_NOMTB_OFF;
  } else {
    led_green_counter_max = 0;
    led_green_counter_mid = 0;
  }

  // režimy blikání žluté led (lokátor)
  if (led_blue_counter == 0) {
    if (state_readdress) {
      led_blue_counter_max = LED_BLUE_READDRESS_ON;
      led_blue_counter_mid = LED_BLUE_READDRESS_OFF;
    } else {
      if (state_beacon) {
        led_blue_counter_max = LED_BLUE_BEACON_ON;
        led_blue_counter_mid = LED_BLUE_BEACON_OFF;
      } else {
        led_blue_counter_max = 0;
        led_blue_counter_mid = 0;
      }
    }
  }

  if (led_red_counter > 0) {
    led_red_counter--;
    /*
    if (((!led_red_flashing) && (led_red_counter == LED_RED_OK_OFF)) ||
      ((led_red_flashing) && (led_red_counter == LED_RED_ERR_OFF)))
      led_red_off();
      */
      if (led_red_counter == 0) led_red_off();
  }
  /*
  if ((led_red_flashing) && (led_red_counter == 0)) {
    led_red_counter = LED_RED_ERR_ON;
    led_red_on();
  }
  */
  
  // zelená - vždy bliká, nebo svítí, nikdy není po tmì
  if (led_gr_counter > 0) {
    led_gr_counter--;
    if (led_gr_counter == led_green_counter_mid) {
      // pokud jsme v pùlce, zhasneme led
      led_green_off();
    }
  } else {
    if (led_green_counter_max > 0) {
      // když má stále blikat, tak obnovíme blikání
      led_gr_counter = led_green_counter_max;
      led_green_on();
    
     } else {
      // když nemá blikat, tak rožneme
      led_green_on();
    }
  }

  // modrá / žlutá - vždy bliká, nikdy nesvítí
  if (led_blue_counter > 0) {
    led_blue_counter--;
    if (led_blue_counter == led_blue_counter_mid) {
      // pokud jsme v pùlce, zhasneme led
      led_blue_off();
    }
  } else {
    if (led_blue_counter_max > 0) {
      // když má stále blikat, tak obnovíme blikání
      led_blue_counter = led_blue_counter_max;
      led_blue_on();
    
     } else {
      // když nemá blikat, tak zhasneme
      led_blue_off();
    }
  }
}

/*****************************************************************************/
void led_red_ok(void) {
  if (led_red_counter == 0) {
    led_red_counter = LED_RED_OK_ON;
    led_red_on();
  }
}

/*****************************************************************************/
/*
void led_blue_ok(void) {
  if (led_blue_counter == 0) {
    led_blue_counter = LED_BLUE_OK;
    led_blue_on();
  }
}
*/

/*****************************************************************************/

