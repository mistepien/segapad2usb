// Author:
//       Michał Stępień <mistepien@wp.pl>
//
// Copyright (c) 2025 Michał Stępień <mistepien@wp.pl>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
#if defined(ARDUINO_AVR_MICRO)

#define DDR_REG_select_Pin DDRD
#define PORT_REG_selectPin PORTD
#define PIN_REG_selectPin PIND

#define DDR_REG_inputPins DDRB
#define PORT_REG_inputPins PORTB
#define PIN_REG_inputPins PINB

#else

#define DDR_REG_select_Pin DDRD
#define PORT_REG_selectPin PORTD
#define PIN_REG_selectPin PIND

#define DDR_REG_inputPins DDRD
#define PORT_REG_inputPins PORTD
#define PIN_REG_inputPins PIND

#endif

enum sega_state {
  SC_CTL_ON = 0,  // The controller is connected
  SC_MODE = 1,    //checking six or three mode 1
  SC_BTN_START = 2,
  SC_DPAD_UP = 3,
  SC_DPAD_DOWN = 4,
  SC_DPAD_LEFT = 5,
  SC_DPAD_RIGHT = 6,
  SC_BTN_A = 7,
  SC_BTN_B = 8,
  SC_BTN_C = 9,
  SC_BTN_X = 10,
  SC_BTN_Y = 11,
  SC_BTN_Z = 12,
  SC_BTN_MODE = 13,
  SC_BTN_HOME = 14  //availabe in M30 2.4 8Bitdo gamepad
};

typedef struct {
  byte _cycle;
  byte _ipin_first;
  byte _ipin_second;
  word _output_bit;
} regs_to_state_struct;

constexpr regs_to_state_struct regs_to_cycle[] = { { 2, 2, 3, bit(SC_CTL_ON) },
                                                   { 2, 4, 4, bit(SC_BTN_A) },
                                                   { 2, 5, 5, bit(SC_BTN_START) },
                                                   { 3, 0, 0, bit(SC_DPAD_UP) },
                                                   { 3, 1, 1, bit(SC_DPAD_DOWN) },
                                                   { 3, 2, 2, bit(SC_DPAD_LEFT) },
                                                   { 3, 3, 3, bit(SC_DPAD_RIGHT) },
                                                   { 3, 4, 4, bit(SC_BTN_B) },
                                                   { 3, 5, 5, bit(SC_BTN_C) },
                                                   { 4, 0, 1, bit(SC_MODE) },
                                                   { 5, 0, 0, bit(SC_BTN_Z) },
                                                   { 5, 1, 1, bit(SC_BTN_Y) },
                                                   { 5, 2, 2, bit(SC_BTN_X) },
                                                   { 5, 3, 3, bit(SC_BTN_MODE) },
                                                   { 6, 0, 0, bit(SC_BTN_HOME) } };

constexpr word three_mode_buttons = bit(SC_CTL_ON) | bit(SC_BTN_START) | bit(SC_DPAD_UP) | bit(SC_DPAD_DOWN) | bit(SC_DPAD_LEFT) | bit(SC_DPAD_RIGHT) | bit(SC_BTN_A) | bit(SC_BTN_B) | bit(SC_BTN_C);

constexpr byte SC_INPUT_PINS = 6;

#define SC_READ_DELAY_MS 0  // Must be >= 3ms to give 6-button controller time to reset

// Delay (µs) between setting the select pin and reading the button pins
#if defined(ARDUINO_AVR_MICRO)
constexpr unsigned long SC_CYCLE_DELAY_US = 6;
#elif (F_CPU == 3686400L) || (F_CPU == 3276800L) || (F_CPU == 2000000L)
constexpr unsigned long SC_CYCLE_DELAY_US = 3;
#elif (F_CPU == 4000000L)
constexpr unsigned long SC_CYCLE_DELAY_US = 4;  //4MHz: 0..5 -- retrobit,   0..20 - 8bido M30 2.4, 0..100 - chinesse no-name
#elif (F_CPU == 6000000L) || (F_CPU == 7372800L) || (F_CPU == 8000000L)
constexpr unsigned long SC_CYCLE_DELAY_US = 7;  //6MHz: 4..11 --retrobit, 7.3728MHz: 0..15 -- retrobit, 8MHZ: 0..13 -- retrobit
#elif (F_CPU == 11059200L) || (F_CPU == 16000000L) || (F_CPU == 12000000L) || (F_CPU == 14745600)
constexpr unsigned long SC_CYCLE_DELAY_US = 10;  //11.0592MHz: 3..18 -- retrobit, 16MHz: 0..17 -- retrobit
#endif

class SegaController {
public:
  void begin(byte db9_pin_7, byte db9_pin_1, byte db9_pin_2, byte db9_pin_3, byte db9_pin_4, byte db9_pin_6, byte db9_pin_9);
  word getState();
  bool complex_bool_value(byte big_byte, byte tested_byte);
private:
  byte _selectPin_bin;  // output select pin useful for HARDWARE XOR
  byte _inputPins[];
  word nod(word _input_state, word _dpad_dirs);
};

extern SegaController sega;
