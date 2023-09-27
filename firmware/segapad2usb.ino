//
// segapad2usb.ino
// Author: mistepien@wp.pl
// Copyright 2023
//
// Based on https://github.com/jonthysell/SegaController
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


/* "PINOUT" of gamepad's output

  0    SC_CTL_ON    = 1, // The controller is connected
  1    SC_MODE      = 2, //checking six or three mode
  2    SC_BTN_START = 4,
  3    SC_BTN_UP    = 8,
  4    SC_BTN_DOWN  = 16,
  5    SC_BTN_LEFT  = 32,
  6    SC_BTN_RIGHT = 64,
  7    SC_BTN_A     = 128,
  8    SC_BTN_B     = 256,
  9    SC_BTN_C     = 512,
  10   SC_BTN_X     = 1024,
  11   SC_BTN_Y     = 2048,
  12   SC_BTN_Z     = 4096,
  13   SC_BTN_HOME  = 8192 //availabe in M30 2.4 8Bitdo gamepad
*/


#include "SegaController.h"
SegaController controller(7, 1, 2, 3, 4, 5, 6);

#include <Joystick.h>  //https://github.com/MHeironimus/ArduinoJoystickLibrary
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK,
                   8, 0,                  // Button Count, Hat Switch Count
                   true, true, false,     // X and Y, but no Z Axis
                   false, false, false,   // No Rx, Ry, or Rz
                   false, false,          // No rudder or throttle
                   false, false, false);  // No accelerator, brake, or steering

void setup() {
  bitSet(DDRD, 0); bitClear(PORTD, 0); //CTL-ON LED
  bitSet(DDRD, 1); bitClear(PORTD, 1); //MODE LED


  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
  Joystick.begin(false);
  Joystick.setXAxis(0);
  Joystick.setYAxis(0);

  delay(1500); /*very ugly and dirty hack
                without that delay() joystick will not
                be centered at the beginning (that is an
                issue with Joystick.sendState();
*/


  /*turn off RX and TX LEDS on permanent basis

     turning off these LEDS here at the end of setup()
     is an indicator that segapad2usb is ready
  */
  bitClear(DDRD, 5);
  bitClear(DDRB, 0);

  Joystick.sendState();
}

byte DPAD_UP;
byte DPAD_DOWN;
byte DPAD_LEFT;
byte DPAD_RIGHT;

void button(byte btn, bool btn_state) {
  switch (btn) {
    case 0:
      bitWrite(PORTD, 0, btn_state); //CTL-ON
      break;
    case 1:
      bitWrite(PORTD, 1, btn_state); //6BTN-MODE
      break;
    case 2:
      Joystick.setButton(0, btn_state); //BTN START
      break;
    case 3:
      DPAD_UP =  btn_state;
      break;
    case 4:
      DPAD_DOWN = btn_state;
      break;
    case 5:
      DPAD_LEFT = btn_state;
      break;
    case 6:
      DPAD_RIGHT = btn_state;
      break;
    case 7:
      Joystick.setButton(1, btn_state); //BTN A
      break;
    case 8:
      Joystick.setButton(2, btn_state); //BTN B
      break;
    case 9:
      Joystick.setButton(3, btn_state); //BTN C
      break;
    case 10:
      Joystick.setButton(4, btn_state); //BTN X
      break;
    case 11:
      Joystick.setButton(5, btn_state); //BTN Y
      break;
    case 12:
      Joystick.setButton(6, btn_state); //BTN Z
      break;
    case 13:
      Joystick.setButton(7, btn_state); //BTN HOME //ONLY M30 8Bitdo
      break;
  }
}

word current_state = 0;
word prev_state = 0;

void send_state() {
  current_state &= 16383; /*&= B0011 1111 1111 1111; CLEAR 2 highest bits
                                                   other way SC_BTN_MODE would trigger
                                                   the routine but we are not interested
                                                   in SC_BTN_MODE at all
*/
  word changed_state = prev_state ^ current_state;
  if ( changed_state ) {
    prev_state = current_state;
    for (byte index = 0; index < 14; index++) {
      if (bitRead(changed_state, index)) {
        button(index, bitRead(current_state, index));
      }
    }
    Joystick.setYAxis(DPAD_DOWN - DPAD_UP);
    Joystick.setXAxis(DPAD_RIGHT - DPAD_LEFT);

  }
  Joystick.sendState(); //ONE common send.State for AXISES AND BUTTONS
}


void loop() {
  while (1) {
    current_state = controller.getState();
    send_state();
  }
}
