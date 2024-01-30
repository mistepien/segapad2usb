//
// SegaController.h (with port registers by mistepien@wp.pl)
//
// Author:
//       Jon Thysell <thysell@gmail.com>
//
// Copyright (c) 2017 Jon Thysell <http://jonthysell.com>
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

enum sega_state {
  SC_CTL_ON = 0,  // The controller is connected
  SC_MODE = 1,    //checking six or three mode 1
  SC_BTN_START = 2,
  SC_BTN_UP = 3,
  SC_BTN_DOWN = 4,
  SC_BTN_LEFT = 5,
  SC_BTN_RIGHT = 6,
  SC_BTN_A = 7,
  SC_BTN_B = 8,
  SC_BTN_C = 9,
  SC_BTN_X = 10,
  SC_BTN_Y = 11,
  SC_BTN_Z = 12,
  SC_BTN_HOME = 13
  //SC_BTN_MODE = 14, //availabe in M30 2.4 8Bitdo gamepad
};

#define DDR_REG_select_Pin DDRD
#define PORT_REG_selectPin PORTD

#define DDR_REG_inputPins DDRB
#define PORT_REG_inputPins PORTB
#define PIN_REG_inputPins PINB

constexpr byte SC_INPUT_PINS = 6;
constexpr byte SC_CYCLES_6BTN = 7; 
constexpr byte SC_CYCLES_3BTN = 5;

constexpr byte SC_READ_DELAY_MS = 4;  // Must be >= 3ms to give 6-button controller time to reset
constexpr byte SC_CYCLE_DELAY_US = 6;  // Delay (µs) between setting the select pin and reading the button pins



class SegaController {
public:
  SegaController(byte db9_pin_7, byte db9_pin_1, byte db9_pin_2, byte db9_pin_3, byte db9_pin_4, byte db9_pin_6, byte db9_pin_9);
  word getState();
private:
  byte readCycle(byte cycle);
  void writeCycle_regs_to_state(byte cycle, byte SEGA_reg);
  word _currentState;
  unsigned long _lastReadTime;
  boolean _sixButtonMode;
  byte _selectPin;  // output select pin
  byte _inputPins[SC_INPUT_PINS];
  bool nod(bool KEY_DIR, bool SECONDARY_DIR);
};

//#endif
