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

// wire it up
// avoid D3 and D4: pulled up.
// avoid D8 it prevents programming and console
byte solenoidPin[4] = {D1, D2, D7, D8};  // D8 is ss, doesn't work?
byte pumpPin[2] = {D5, D6};
#define ON LOW
#define OFF HIGH
// also used
// D4, GPIO2, BUILTIN_LED

String myRole;

void setup() {

  // set them off, then enable pin.
  for ( byte i = 0; i < 4; i++ ) {
    digitalWrite(solenoidPin[i], OFF); pinMode(solenoidPin[i], OUTPUT);
    if( i<2 ) digitalWrite(pumpPin[i], OFF); pinMode(pumpPin[i], OUTPUT);
  }

  delay(250); // wait a tick

    // for random numbers
  randomSeed(analogRead(0));

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( true ) Comms.saveStuff("whichBox", "gwf/a/pump/A");
//  if ( true ) Comms.saveStuff("whichBox", "gwf/a/pump/B");

  myRole = Comms.loadStuff("whichBox");
  Serial << "Startup: my role [" << myRole << "]." << endl;

  // configure comms
  Comms.begin(myRole, processMessages);
  Comms.sub(myRole + "/#"); // all my messages

  boolean state = ON;
  
  digitalWrite(solenoidPin[0], state);
  digitalWrite(solenoidPin[1], state);
  digitalWrite(solenoidPin[2], state);
  digitalWrite(solenoidPin[3], state);
  digitalWrite(pumpPin[0], state);
  digitalWrite(pumpPin[1], state);
  
  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // pump manipulation.  KISS.

  Serial << "Topic: " << topic << " msg: " << message << endl;
  Serial << "binary: 1: " << Comms.messageBinary[0] << " 1: " << Comms.messageBinary[1] << endl;
  byte pin;
  if( topic.indexOf("sprayers") != -1 ) {
    if ( topic.endsWith("A") ) pin = solenoidPin[0]; 
    else if ( topic.endsWith("B") ) pin = solenoidPin[1];
    else if ( topic.endsWith("C") ) pin = solenoidPin[2];
    else if ( topic.endsWith("D") ) pin = solenoidPin[3];
    else return;
    
  } else if ( topic.indexOf("pumps") != -1 ) {
    if ( topic.endsWith("A") ) pin = pumpPin[0]; 
    else if ( topic.endsWith("B") ) pin = pumpPin[1];
    else return;    
  }

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF; // is on?
  // note the specific requirement to be ON.  
    
  // Do it.
  digitalWrite(pin, setting);
  Serial << "Setting pin " << pin << "=" << setting << endl;
  
}
