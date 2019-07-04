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
#define RELAY2 2
#define RELAY3 3
#define RELAY4 4

#define MASTERB 18
#define RED MASTERB+1
#define GRN MASTERB+2
#define BLU MASTERB+3
#define WHT MASTERB+4

#define FOG_PIN D1

#include <Metro.h>
Metro cycleTime(5000UL);

void setup() {
  Serial.begin(115200);
  
  digitalWrite(FOG_PIN,0);
  pinMode(FOG_PIN,OUTPUT);
  
  dmx.init();               // initialization for first 32 addresses by default
  //dmx.init(512)           // initialization for complete bus
  delay(200);               // wait a while (not necessary)
  
  dmx.write(MASTERB, 255);        // channal on

}

void loop() {
/*
  byte i = 0;
  while(1) {
    yield();
    dmx.write(RELAY1, i++);
    dmx.update();           // update the DMX bus
  }
  */
  Serial.println("Red. 1");
  dmx.write(RED, 255); dmx.write(GRN, 0); dmx.write(BLU, 0);  dmx.write(WHT, 0); 
  dmx.write(RELAY1, 255); dmx.write(RELAY2, 0); dmx.write(RELAY3, 0); dmx.write(RELAY4, 0);
//  digitalWrite(FOG_PIN,1);
  while(!cycleTime.check()) {
    dmx.update();           // update the DMX bus
    yield();
  }

  Serial.println("Grn. 2");
  dmx.write(RED, 0); dmx.write(GRN, 255); dmx.write(BLU, 0);  dmx.write(WHT, 0); 
  dmx.write(RELAY1, 0); dmx.write(RELAY2, 255); dmx.write(RELAY3, 0); dmx.write(RELAY4, 0);
  digitalWrite(FOG_PIN,0);
  while(!cycleTime.check()) {
    dmx.update();           // update the DMX bus
    yield();
  }

  Serial.println("Blu. 3");
  dmx.write(RED, 0); dmx.write(GRN, 0); dmx.write(BLU, 255);  dmx.write(WHT, 0); 
  dmx.write(RELAY1, 0); dmx.write(RELAY2, 0); dmx.write(RELAY3, 255); dmx.write(RELAY4, 0);
//  digitalWrite(FOG_PIN,1);
  while(!cycleTime.check()) {
    dmx.update();           // update the DMX bus
    yield();
  }

  Serial.println("Blk. 4");
  dmx.write(RED, 0); dmx.write(GRN, 0); dmx.write(BLU, 0); dmx.write(WHT, 0); 
  dmx.write(RELAY1, 0); dmx.write(RELAY2, 0); dmx.write(RELAY3, 0); dmx.write(RELAY4, 255);
  digitalWrite(FOG_PIN,0);
  while(!cycleTime.check()) {
    dmx.update();           // update the DMX bus
    yield();
  }

  Serial.println("Wht. 5");
  dmx.write(RED, 0); dmx.write(GRN, 0); dmx.write(BLU, 0); dmx.write(WHT, 255);
  dmx.write(RELAY1, 0); dmx.write(RELAY2, 0); dmx.write(RELAY3, 0); dmx.write(RELAY4, 255);
  digitalWrite(FOG_PIN,0);
  while(!cycleTime.check()) {
    dmx.update();           // update the DMX bus
    yield();
  }

}
