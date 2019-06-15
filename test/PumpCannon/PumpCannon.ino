// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

/*
 * Messages:
 * 
 * gwf/a/pumpcannon/pump=0
 * gwf/a/pumpcannon/pump=1
 * 
 * gwf/a/pumpcannon/cannon=0
 * gwf/a/pumpcannon/cannon=1
 * 
 */

// wire it up

// Assuming we have two pumps hooked up
byte pumpPin1 = D1;
byte pumpPin2 = D2;

// Assuming we have one solenoid hooked up
byte solenoidPin1 = D5;

#define ON HIGH
#define OFF LOW

String myRole = "gwf/a/pumpcannon";

void setup() {

  // set them off, then enable pin.
  digitalWrite(pumpPin1, OFF); pinMode(pumpPin1, OUTPUT);
  digitalWrite(pumpPin2, OFF); pinMode(pumpPin2, OUTPUT);
  digitalWrite(solenoidPin1, OFF); pinMode(solenoidPin1, OUTPUT);

  delay(250); // wait a tick

  // for random numbers
  randomSeed(analogRead(0));

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.

  Serial << "Startup: my role [" << myRole << "]." << endl;

  // configure comms
  Comms.begin(myRole, processMessages);
  Comms.sub(myRole + "/#"); // all my messages

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // pump manipulation.  KISS.

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF; // is on?

  byte pin;
  if ( topic.indexOf("pump") != -1 ) {
    digitalWrite(pumpPin1, setting);
    digitalWrite(pumpPin2, setting);
  } else if ( topic.indexOf("cannon") != -1 ) {    
    digitalWrite(solenoidPin1, setting);
  }

}
