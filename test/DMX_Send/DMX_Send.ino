// - - - - -
// ESPDMX - A Arduino library for sending and receiving DMX using the builtin serial hardware port.
//
// Copyright (C) 2015  Rick <ricardogg95@gmail.com>
// This work is licensed under a GNU style license.
//
// Last change: Musti <https://github.com/IRNAS> (edited by Musti)
//
// Documentation and samples are available at https://github.com/Rickgg/ESP-Dmx
// Connect GPIO02 - TDX1 to MAX3485 or other driver chip to interface devices
// Pin is defined in library
// - - - - -

#include <ESPDMX.h>

DMXESPSerial dmx;

#define RELAY1 1

#define MASTERB 18
#define RED MASTERB+1
#define GRN MASTERB+2
#define BLU MASTERB+3
#define WHT MASTERB+4

void setup() {
  dmx.init();               // initialization for first 32 addresses by default
  //dmx.init(512)           // initialization for complete bus
  delay(200);               // wait a while (not necessary)
  dmx.write(MASTERB, 255);        // channal on
  dmx.update();           // update the DMX bus
}

void loop() {

  dmx.write(RED, 255);
  dmx.write(RELAY1, 255);
  dmx.update();           // update the DMX bus
  delay(3000);            // wait for 1s

  dmx.write(RED, 0);
  dmx.write(RELAY1, 0);
  dmx.update();
  delay(3000);

}
