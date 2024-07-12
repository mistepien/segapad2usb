// SegaController.h (with port registers, faster reading loop and lots of optimalization)
//
// Author:
//       Jon Thysell <thysell@gmail.com>
//       Michał Stępień <mistepien@wp.pl>
//
// Copyright (c) 2017 Jon Thysell <http://jonthysell.com>
// Copyright (c) 2023 Michał Stępień <mistepien@wp.pl>
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

#include "Arduino.h"
#include "SegaController.h"

SegaController::SegaController(byte db9_pin_7, byte db9_pin_1, byte db9_pin_2, byte db9_pin_3, byte db9_pin_4, byte db9_pin_6, byte db9_pin_9) {
  // Set pins
  _selectPin = db9_pin_7;
  _inputPins[0] = db9_pin_1;
  _inputPins[1] = db9_pin_2;
  _inputPins[2] = db9_pin_3;
  _inputPins[3] = db9_pin_4;
  _inputPins[4] = db9_pin_6;
  _inputPins[5] = db9_pin_9;

  bitSet(DDR_REG_select_Pin, _selectPin);  // Setup selectPin as OUTPUT
  //bitClear(PORT_REG_selectPin, _selectPin); // Setup selectPin -- default LOW is ok

  /* Setup input pins

  Default state of PINS is INPUT -- without INPUT_PULLUP
  INPUT_PULLUP was not enough reliable in case of ATMEGA48PA
  For MDJOY PCB 10K Ohm pull-up resistors are ok.
  Thus _inputPin[x] are in INPUT (not INPUT_PULLUP) by default
  */

#if defined(ARDUINO_AVR_MICRO)
  byte _all_inputPins = 0;
  for (byte i = 0; i < SC_INPUT_PINS; i++) {
    _all_inputPins |= bit(_inputPins[i]);
  }
  //DDR_REG_inputPins &= ~_all_inputPins; //INPUT by default
  PORT_REG_inputPins |= _all_inputPins;
#endif
}

word SegaController::getState() {
  static word _currentState;
  static unsigned long _lastReadTime;
  if ((millis() - _lastReadTime) < SC_READ_DELAY_MS) {
    // Not enough time has elapsed, return previously read state
    return _currentState;
  }


  //changing number of cycles depending is it 3BTN-mode or 6BTN-mode
  byte SC_CYCLES = SC_CYCLES_3BTN;
  if (bitRead(_currentState, SC_MODE)) {  //_currentState is not yet cleared
    SC_CYCLES = SC_CYCLES_6BTN;
  }

  byte _readCycle_regs[8];
  byte _selectPin_bin = bit(_selectPin);

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
  For instance SNES protocol is much better.
  */
#pragma GCC unroll 8
  for (byte cycle = 0; cycle < 8; cycle++) {
    _delay_us(SC_CYCLE_DELAY_US);
    _readCycle_regs[cycle] = PIN_REG_inputPins;
    PIN_REG_selectPin = _selectPin_bin;  //hardware XOR
  }

  interrupts();

  //Here slowness is not an issue any more since reading of Sega gamepad is already done.
  //What happens is processing of already collected data and producing output.

  _currentState = 0;  // Clear current state
  for (byte cycle = 0; cycle < SC_CYCLES; cycle++) {
    _currentState |= writeCycle_regs_to_state(cycle, _readCycle_regs[cycle]);
    if (1 ^ bitRead(_currentState, SC_CTL_ON)) {
      break;
      /* When a controller disconnects, revert to three-button polling
        and stop proccessing controller output.
        Since SC_CTL_ON is pushed into _currentState in cycle=0
        then _currentState=0 is if (cycle=0 and SC_CTL_ON=0).
        _currentState is  cleared just before the for loop.
      */
    }
  }
  _lastReadTime = millis();

  return _currentState;
}

word SegaController::writeCycle_regs_to_state(byte cycle, byte SEGAinput) {
  word _cycle_state = 0;
  SEGAinput = ~(SEGAinput);  //switch from logic {LOW=pressed, HIGH=released} to logic {HIGH=pressed, LOW=released}
  switch (cycle) {
    case 0:
      // Check that a controller is connected // getState() imidiatelly returns 0 and does not read other cases
      if ((bitRead(SEGAinput, _inputPins[2])) && (bitRead(SEGAinput, _inputPins[3]))) {
        bitSet(_cycle_state, SC_CTL_ON);
      }
      break;
    case 2:
      // Read input pins for A, Start
      if (bitRead(SEGAinput, _inputPins[4])) {
        bitSet(_cycle_state, SC_BTN_A);
      }
      if (bitRead(SEGAinput, _inputPins[5])) {
        bitSet(_cycle_state, SC_BTN_START);
      }
      break;
    case 1:  //it can be also 3 -- the same result
      // Read input pins for Up, Down, Left, Right, B, C
      if (bitRead(SEGAinput, _inputPins[0])) {
        bitSet(_cycle_state, SC_DPAD_UP);
      }
      if (bitRead(SEGAinput, _inputPins[1])) {
        bitSet(_cycle_state, SC_DPAD_DOWN);
      }
      if (bitRead(SEGAinput, _inputPins[2])) {
        bitSet(_cycle_state, SC_DPAD_LEFT);
      }
      if (bitRead(SEGAinput, _inputPins[3])) {
        bitSet(_cycle_state, SC_DPAD_RIGHT);
      }
      if (bitRead(SEGAinput, _inputPins[4])) {
        bitSet(_cycle_state, SC_BTN_B);
      }
      if (bitRead(SEGAinput, _inputPins[5])) {
        bitSet(_cycle_state, SC_BTN_C);
      }
      _cycle_state = nod_dpad(_cycle_state);  //NO OPPOSITE DIRECTIONS
      break;
    case 4:
      if (bitRead(SEGAinput, _inputPins[0]) && bitRead(SEGAinput, _inputPins[1])) {
        bitSet(_cycle_state, SC_MODE);
      }
      break;
    case 5:
      // Read input pins for X, Y, Z, Mode
      if (bitRead(SEGAinput, _inputPins[0])) {
        bitSet(_cycle_state, SC_BTN_Z);
      }
      if (bitRead(SEGAinput, _inputPins[1])) {
        bitSet(_cycle_state, SC_BTN_Y);
      }
      if (bitRead(SEGAinput, _inputPins[2])) {
        bitSet(_cycle_state, SC_BTN_X);
      }
      /*if (bitRead(SEGAinput, _inputPins[3])) {
         bitSet(_currentState, SC_BTN_MODE);
        }
      */
      break;
    case 6:
      // Read input pins for HOME button -- that is a feature of 8Bitdo M30 2.4
      if (bitRead(SEGAinput, _inputPins[0])) {
        bitSet(_cycle_state, SC_BTN_HOME);
      }
      break;
  }
  return _cycle_state;
}

word SegaController::nod_dpad(word _gamepadState) {
  bool SC_DPAD_UP_tmp = nod(bitRead(_gamepadState, SC_DPAD_UP), bitRead(_gamepadState, SC_DPAD_DOWN));
  bool SC_DPAD_DOWN_tmp = nod(bitRead(_gamepadState, SC_DPAD_DOWN), bitRead(_gamepadState, SC_DPAD_UP));
  bool SC_DPAD_LEFT_tmp = nod(bitRead(_gamepadState, SC_DPAD_LEFT), bitRead(_gamepadState, SC_DPAD_RIGHT));
  bool SC_DPAD_RIGHT_tmp = nod(bitRead(_gamepadState, SC_DPAD_RIGHT), bitRead(_gamepadState, SC_DPAD_LEFT));
  bitWrite(_gamepadState, SC_DPAD_UP, SC_DPAD_UP_tmp);
  bitWrite(_gamepadState, SC_DPAD_DOWN, SC_DPAD_DOWN_tmp);
  bitWrite(_gamepadState, SC_DPAD_LEFT, SC_DPAD_LEFT_tmp);
  bitWrite(_gamepadState, SC_DPAD_RIGHT, SC_DPAD_RIGHT_tmp);
  return _gamepadState;
}

bool SegaController::nod(bool OUT_DIR, bool SECOND_DIR) {
  //bool value = ((!( OUT_DIR  || SECOND_DIR )) || OUT_DIR); //logic where LOW=pressed, HIGH=released
  bool value = ((!(OUT_DIR && SECOND_DIR)) && OUT_DIR);  //logic where HIGH=pressed, LOW=released
  return value;
}
