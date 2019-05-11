// IDE Settings:
// Tools->Board : "WeMos D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

// wire it up
// devices with the light shield have access to D5-D8
byte pin[4] = {D5, D6, D7, D8};
#define ON HIGH
#define OFF LOW
// also used
// D4, GPIO2, BUILTIN_LED

String myRole;

void setup() {
  // for random numbers
  randomSeed(analogRead(0));

  // set them off, then enable pin.
  for ( byte i = 0; i < 4; i++ ) {
    digitalWrite(pin[i], OFF); pinMode(pin[i], OUTPUT);
  }

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( true ) {
    Comms.saveStuff("whichBox", "Prime_Cannon_Sprinkler");
    Comms.saveStuff("whichBox", "Boost_Sprinkler_Sprinkler");
  }
  myRole = Comms.loadStuff("whichBox");
  Serial << "Startup: my role [" << myRole << "]." << endl;

  // configure comms
  if ( myRole.equals("Prime_Cannon_Sprinkler") ) {
    Comms.begin(myRole, primeCannonSprinkler);
    Comms.sub(Comms.actPump[0]); // prime A
    Comms.sub(Comms.actPump[1]); // prime B
    Comms.sub(Comms.actPump[4]); // cannon
    Comms.sub(Comms.actBeaconSpray[0]); // spray A
  }
  else {
    Comms.begin(myRole, boostSprinklerSprinkler);
    Comms.sub(Comms.actPump[2]); // boost A
    Comms.sub(Comms.actPump[3]); // boost B
    Comms.sub(Comms.actBeaconSpray[1]); // spray B
    Comms.sub(Comms.actBeaconSpray[2]); // spray C
  }

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void primeCannonSprinkler(String topic, String message) {

  // pump manipulation.  KISS.

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]); // is on?
  if( setting ) setting = ON;
  else setting = OFF;
  
  byte pin;
  if ( topic.equals(Comms.actPump[0]) ) pin = D5; // prime A
  else if ( topic.equals(Comms.actPump[1]) ) pin = D6; // prime B
  else if ( topic.equals(Comms.actPump[4]) ) pin = D7; // cannon
  else if ( topic.equals(Comms.actBeaconSpray[0]) ) pin = D8; // spray A
  else return;

  digitalWrite(pin, setting);
}

// processes messages that arrive
void boostSprinklerSprinkler(String topic, String message) {

  // pump manipulation.  KISS.

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]); // is on?

  byte pin;
  if ( topic.equals(Comms.actPump[2]) ) pin = D5; // boost A
  else if ( topic.equals(Comms.actPump[3]) ) pin = D6; // boost B
  else if ( topic.equals(Comms.actBeaconSpray[1]) ) pin = D7; // spray B
  else if ( topic.equals(Comms.actBeaconSpray[2]) ) pin = D8; // spray C
  else return;

  digitalWrite(pin, setting);
}
