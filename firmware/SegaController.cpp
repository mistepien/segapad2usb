/*********************************************
   Created by Michał Stępień
   Contact: mistepien@wp.pl
   License: GPLv3
 *********************************************/
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

/*----------------------------------------
  |   DDRx   |   PORTx  |    result      |
  ---------------------------------------|
  ---------------------------------------|
  |    0     |     0    |     INPUT      |
  ----------------------------------------
  |    0     |     1    |  INPUT_PULLUP  |
  ----------------------------------------
  |    1     |     0    | OUTPUT (LOW)   |
  ----------------------------------------
  |    1     |     1    | OUTPUT (HIGH)  |
  ----------------------------------------

  LOW(0) state of both registers is DEFAULT,
  thus every pin is in INPUT mode without doing anything.

  ------------------            -----------------
  |  HARDWARE XOR  |            |  SOFTWARE XOR |
  ------------------            -----------------
  PINx = byte;    <========>    PORTx ^= byte;

  ------------------
  |  HARDWARE XOR  |
  ------------------

  "The port input pins I/O location is read only, while the data register and the
  data direction register are read/write. However, writing a logic one to a bit
  in the PINx register, will result in a toggle in the
  corresponding bit in the data register."

  That is more efficient since XOR operation is done in hardware, not software,
  and it saves cycles since in code there is no need to bother about XOR.*/

//https://retroramblings.net/?p=835 was very helpful

#include "Arduino.h"
#if defined(__AVR_ATtiny88__) || defined(__AVR_ATtiny48__)
#include <util/delay.h>
#endif
#include "SegaController.h"

#define _nop() __asm__ volatile("nop")

void SegaController::begin(byte db9_pin_7, byte db9_pin_1, byte db9_pin_2, byte db9_pin_3, byte db9_pin_4, byte db9_pin_6, byte db9_pin_9) {
  // Set pins
  byte _selectPin = db9_pin_7;
  _selectPin_bin = bit(_selectPin);
  _inputPins[0] = bit(db9_pin_1);
  _inputPins[1] = bit(db9_pin_2);
  _inputPins[2] = bit(db9_pin_3);
  _inputPins[3] = bit(db9_pin_4);
  _inputPins[4] = bit(db9_pin_6);
  _inputPins[5] = bit(db9_pin_9);


  bitSet(DDR_REG_select_Pin, _selectPin);  // Setup selectPin as OUTPUT
  bitSet(PORT_REG_selectPin, _selectPin);  // Setup selectPin as HIGH


  /* Setup input pins

  Default state of PINS is INPUT -- without INPUT_PULLUP
  INPUT_PULLUP was not enough reliable in case of ATMEGA48PA
  For MDJOY PCB 15K Ohm pull-up resistors are ok.
  Thus _inputPin[x] are in INPUT (not INPUT_PULLUP) by default
  */


#if defined(ARDUINO_AVR_MICRO)
  byte _all_inputPins = 0;
  for (byte i = 0; i < SC_INPUT_PINS; i++) {
    _all_inputPins |= _inputPins[i];
  }
  //DDR_REG_inputPins &= ~_all_inputPins; //INPUT by default
  PORT_REG_inputPins |= _all_inputPins;  //INPUT_PULLUP
#endif
}

word SegaController::getState() {


#if SC_READ_DELAY_MS > 0
  static word _currentState;
  static unsigned long _lastReadTime;
  if ((millis() - _lastReadTime) < SC_READ_DELAY_MS) {
    // Not enough time has elapsed, return previously read state
    return _currentState;
  }
#else
  word _currentState = 0;
#endif


  byte _readCycle_regs[8];

  noInterrupts();

/*Here is the most time sensitive piece of code.
  Being slow means failure in communication  with gamepad.
  SEGA gamepad readings of port registers are dumped to the byte array.
  It works like a kind of shiftIn -- but output is not a byte but
  an array of bytes.

  That's all folks!

  BTW, from technical point of view Sega Genesis 6button game pad
  protocol is just a piece of crap. Expansion from 3button to 6button
  made it very dependent upon very accurate delay :D
  For instance SNES gamepad protocol is much better.
  */
#pragma GCC unroll 8
  for (byte cycle = 0; cycle < 8; cycle++) {
    PIN_REG_selectPin = _selectPin_bin;  //hardware XOR
#if ((F_CPU == 500000L) || (F_CPU == 1000000L))
    _nop();
#else
    _delay_us(SC_CYCLE_DELAY_US);
#endif
    _readCycle_regs[cycle] = PIN_REG_inputPins;
  }

  interrupts();

  /*Here slowness is not an issue any more since reading of Sega gamepad is already done.
  What happens is processing of already collected data and producing output.*/

#if SC_READ_DELAY_MS > 0
  _currentState = 0;  // Clear current state if _currentState is static
#endif

  for (byte i = 0; i < 15; i++) {
    byte __input_pins_bits__ = _inputPins[regs_to_cycle[i]._ipin_first] | _inputPins[regs_to_cycle[i]._ipin_second];
    byte __cycle__ = ~(_readCycle_regs[regs_to_cycle[i]._cycle]);
    if (complex_bool_value(__cycle__, __input_pins_bits__)) {
      _currentState |= regs_to_cycle[i]._output_bit;
    }
  }

  if (!(_currentState & bit(SC_MODE))) {  //in 3-button mode buttons X,Y,Z,MODE and HOME are never pressed
    _currentState &= three_mode_buttons;
  }

  _currentState = nod(_currentState, bit(SC_DPAD_UP) | bit(SC_DPAD_DOWN));     //NO OPPOSITE DIRECTIONS
  _currentState = nod(_currentState, bit(SC_DPAD_RIGHT) | bit(SC_DPAD_LEFT));  //NO OPPOSITE DIRECTIONS

  _currentState = _currentState & bit(SC_CTL_ON) ? _currentState : 0;

  /*end of puting MD state into one word variable (_current)*/


#if SC_READ_DELAY_MS > 0
  _lastReadTime = millis();
#endif

  return _currentState;
}


word SegaController::nod(word _input_state, word _dpad_dirs) {
  word xor_output = (_input_state & _dpad_dirs) ^ _dpad_dirs;
  return xor_output ? _input_state : (_input_state & ~(_dpad_dirs));
}

bool SegaController::complex_bool_value(byte big_byte, byte tested_byte) {
  bool ___output = (big_byte & tested_byte) ^ tested_byte ? 0 : 1;
  return ___output;
}

SegaController sega;
