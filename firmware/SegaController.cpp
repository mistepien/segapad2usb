//
// SegaController.h (with port registers and faster reading loop)
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

  byte _all_inputPins = 0;
  for (byte i = 0; i < SC_INPUT_PINS; i++) {
    _all_inputPins |= bit( _inputPins[i]);
  }

  bitSet(DDR_REG_select_Pin, _selectPin); // Setup selectPin as OUTPUT
  bitSet(PORT_REG_selectPin, _selectPin); // Setup selectPin as HIGH

  // Setup input pins
#if defined(ARDUINO_ARCH_AVR)
  /*  bitSet(PORT_REG_inputPins,_inputPins[i]); (INPUT_PULLUP) ===> internal PULL-UP
  */
  DDR_REG_inputPins &=  ~_all_inputPins;
  PORT_REG_inputPins |=   _all_inputPins;

#else
  /* bitClear(PORT_REG_inputPins,_inputPins[i]); (INPUT, not INPUT_PULLUP)  --> external PULL-UP
     INPUT_PULLUP was not enough reliable in case of ATMEGA48PA
     For MDJOY PCB 10K Ohm pull-up resistors are ok.
  */
  DDR_REG_inputPins &=  ~_all_inputPins;
  PORT_REG_inputPins &= ~_all_inputPins;
#endif

  _currentState = 0;
  _sixButtonMode = false;
  _lastReadTime = 0;
}

word SegaController::getState() {
  if ((millis() - _lastReadTime) < SC_READ_DELAY_MS) {
    // Not enough time has elapsed, return previously read state
    return _currentState;
  }

  // Clear current state
  _currentState = 0;
  byte _readCycle_regs[SC_CYCLES_6BTN];

  //changing number of cycles depending is it 3BTN-mode or 6BTN-mode
  byte SC_CYCLES = SC_CYCLES_3BTN;
  if (_sixButtonMode) {
    SC_CYCLES = SC_CYCLES_6BTN;
  }

  noInterrupts();

  //This is here the most time sensitive piece of code
  //slowness means failure in communication  with gamepad
  //SEGA gamepad readings of port registers are dumped to the byte array
  //It works like a kind of shiftIn -- but output is not a byte
  //an array of bytes
  //That's all folks

  for (byte cycle = 0; cycle < SC_CYCLES; cycle++) {
    _readCycle_regs[cycle] = readCycle(cycle);
  }

  interrupts();

  //Here slowness is not an issue any more since reading of Sega gamepad is over
  //What happens is processing of already collected data and producing output
  for (byte cycle = 0; cycle < SC_CYCLES - 2; cycle++) {
    writeCycle_regs_to_state(cycle + 2, _readCycle_regs[cycle + 2]);
  }
  _lastReadTime = millis();

  return _currentState;
}

byte SegaController::readCycle(byte cycle) {

  // Set the select pin low/high
  bitWrite(PORT_REG_selectPin, _selectPin, cycle & 1);

  byte SEGA_reg = 0;

  switch (cycle) {
    case 0:
    case 1:
      break;
    default :
      //"stabilizing" gamepad
      delayMicroseconds(SC_CYCLE_DELAY_US);
      SEGA_reg = PIN_REG_inputPins;
  }

  return SEGA_reg;
}

void SegaController::writeCycle_regs_to_state(byte cycle, byte SEGAinput) {
  SEGAinput = ~(SEGAinput); //switch from logic {LOW=pressed, HIGH=released} to logic {HIGH=pressed, LOW=released}
  switch (cycle) {
    case 2:
      // Check that a controller is connected
      if ((bitRead(SEGAinput, _inputPins[2]) && bitRead(SEGAinput, _inputPins[3]))) {
        bitSet(_currentState, SC_CTL_ON);
        // Read input pins for A, Start
        if (bitRead(SEGAinput, _inputPins[4])) {
          bitSet(_currentState, SC_BTN_A);
        }
        if (bitRead(SEGAinput, _inputPins[5])) {
          bitSet(_currentState, SC_BTN_START);
        }
      }
      break;
    case 3:
      if (bitRead(_currentState, SC_CTL_ON)) {
        // Read input pins for Up, Down, Left, Right, B, C
        if (bitRead(SEGAinput, _inputPins[0])) {
          bitSet(_currentState, SC_BTN_UP);
        }
        if (bitRead(SEGAinput, _inputPins[1])) {
          bitSet(_currentState, SC_BTN_DOWN);
        }
        if (bitRead(SEGAinput, _inputPins[2])) {
          bitSet(_currentState, SC_BTN_LEFT);
        }
        if (bitRead(SEGAinput, _inputPins[3])) {
          bitSet(_currentState, SC_BTN_RIGHT);
        }
        if (bitRead(SEGAinput, _inputPins[4])) {
          bitSet(_currentState, SC_BTN_B);
        }
        if (bitRead(SEGAinput, _inputPins[5])) {
          bitSet(_currentState, SC_BTN_C);
        }

        //NO OPPOSITE DIRECTIONS
        bool SC_BTN_UP_tmp = nod(bitRead(_currentState, SC_BTN_UP), bitRead(_currentState, SC_BTN_DOWN));
        bool SC_BTN_DOWN_tmp = nod(bitRead(_currentState, SC_BTN_DOWN), bitRead(_currentState, SC_BTN_UP));
        bool SC_BTN_LEFT_tmp = nod(bitRead(_currentState, SC_BTN_LEFT), bitRead(_currentState, SC_BTN_RIGHT));
        bool SC_BTN_RIGHT_tmp = nod(bitRead(_currentState, SC_BTN_RIGHT), bitRead(_currentState, SC_BTN_LEFT));
        bitWrite(_currentState, SC_BTN_UP, SC_BTN_UP_tmp);
        bitWrite(_currentState, SC_BTN_DOWN, SC_BTN_DOWN_tmp);
        bitWrite(_currentState, SC_BTN_LEFT, SC_BTN_LEFT_tmp);
        bitWrite(_currentState, SC_BTN_RIGHT, SC_BTN_RIGHT_tmp);
      }
      break;
    case 4:
      if (bitRead(_currentState, SC_CTL_ON)) {  // When a controller disconnects, revert to three-button polling
        _sixButtonMode = (bitRead(SEGAinput, _inputPins[0]) && bitRead(SEGAinput, _inputPins[1]));
      } else {
        _sixButtonMode = false;
      }
      break;
    case 5:
      if (_sixButtonMode) {
        bitSet(_currentState, SC_MODE);
        // Read input pins for X, Y, Z, Mode
        if (bitRead(SEGAinput, _inputPins[0])) {
          bitSet(_currentState, SC_BTN_Z);
        }
        if (bitRead(SEGAinput, _inputPins[1])) {
          bitSet(_currentState, SC_BTN_Y);
        }
        if (bitRead(SEGAinput, _inputPins[2])) {
          bitSet(_currentState, SC_BTN_X);
        }
        /*if (bitRead(SEGAinput, _inputPins[3])) {
           bitSet(_currentState, SC_BTN_MODE);
          }
        */
      }
      break;
    case 6:
      if (_sixButtonMode) {
        // Read input pins for HOME button -- that is a feature of 8Bitdo M30 2.4
        if (bitRead(SEGAinput, _inputPins[0])) {
          bitSet(_currentState, SC_BTN_HOME);
        }
      }
      break;
  }
}

bool SegaController::nod(bool OUT_DIR, bool SECOND_DIR) {
  //bool value = ((!( OUT_DIR  || SECOND_DIR )) || OUT_DIR); //logic where LOW=pressed, HIGH=released
  bool value = ((!( OUT_DIR  && SECOND_DIR )) && OUT_DIR); //logic where HIGH=pressed, LOW=released
  return value;
}
