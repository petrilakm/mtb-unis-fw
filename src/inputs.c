#include <stddef.h>
#include "inputs.h"
#include "io.h"
#include "config.h"

uint16_t inputs_logic_state = 0;
uint16_t inputs_debounced_state = 0;
uint16_t inputs_old = 0;

bool btn_pressed = false;

bool io_button_short_pressed; // turn high after short press
bool io_button_long_pressed; // turn high after long press

// timebase 2 kHz
#define DEBOUNCE_THRESHOLD 20 // 10 ms
uint8_t _inputs_debounce_counter[NO_INPUTS] = {0, };
uint8_t _inputs_fall_counter[NO_INPUTS] = {0, };
uint8_t _btn_debounce_counter = 0;
 // button long press > 2 s
#define BTN_PRESSED_THRESHOLD (200)
uint8_t _btn_pressed_counter = 0;


static void _inputs_button_debounce_update();

void inputs_debounce_update(void) {
  // 2 kHz
  uint16_t state = io_get_inputs_raw();

  for (size_t i = 0; i < NO_INPUTS; i++) {
    if (state & 0x01) { // state == 1 → logical 0
      if (_inputs_debounce_counter[i] > 0) {
        _inputs_debounce_counter[i]--;
        // If you need evens in future, you must enter if below only if
        // inputs_debounced_state[i] == 1
        if (_inputs_debounce_counter[i] == 0) {
          inputs_debounced_state &= ~(1 << i);
          _inputs_fall_counter[i] = input_delay(i)*10;
          if (_inputs_fall_counter[i] == 0)
            inputs_logic_state &= ~(1 << i);
        }
      }
    } else {
      if (_inputs_debounce_counter[i] < DEBOUNCE_THRESHOLD) {
        _inputs_debounce_counter[i]++;
        // If you need evens in future, you must enter if below only if
        // inputs_debounced_state[i] == 0
        if (_inputs_debounce_counter[i] == DEBOUNCE_THRESHOLD) {
          inputs_debounced_state |= (1 << i);
          inputs_logic_state |= (1 << i); // no fall
          if (_inputs_fall_counter[i] > 0)
            _inputs_fall_counter[i] = 0; // stop falling counter
        }
      }
    }

    state >>= 1;
  }

  _inputs_button_debounce_update();
}

void inputs_fall_update(void) {
  // Called at 100 Hz (each 10 ms)
  for (size_t i = 0; i < NO_INPUTS; i++) {
    if (_inputs_fall_counter[i] > 0) {
      _inputs_fall_counter[i]--;
      if (_inputs_fall_counter[i] == 0)
        inputs_logic_state &= ~(1 << i);
    }
  }
}

void button_long_press_detect_update(void) {
  // Called at 100 Hz (each 10 ms)
  // determine if press is short or long
  if (btn_pressed) {
    if (_btn_pressed_counter < BTN_PRESSED_THRESHOLD) {
      _btn_pressed_counter++;
      if (_btn_pressed_counter == BTN_PRESSED_THRESHOLD) {
        io_button_long_pressed = true;
      }
    }
   } else {
    _btn_pressed_counter = 0;
  }
}

static void _inputs_button_debounce_update(void) {
  bool state = io_button();
  if (state & 0x01) {
    // released
    if (_btn_debounce_counter > 0) {
      _btn_debounce_counter--;
      if ((_btn_debounce_counter) == 0 && (btn_pressed)) {
        btn_pressed = false;
        if (_btn_pressed_counter < BTN_PRESSED_THRESHOLD) {
          io_button_short_pressed = true;
        }
      }
    }
  } else {
    // pressed
    if (_btn_debounce_counter < DEBOUNCE_THRESHOLD) {
      _btn_debounce_counter++;
      if ((_btn_debounce_counter == DEBOUNCE_THRESHOLD) && (!btn_pressed)) {
        btn_pressed = true;
      }
    }
  }
}
