// IDE Settings:
// Tools->Board : "WeMos D1 R2 & mini"
// Tools->Flash Size : "4M (3M SPIFFS)"
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
const byte pin[4] = {D5, D6, D7, D8};
// pump box 1: pump0, pump1, N/A, N/A
// pump box 2: pump0, pump1, N/A, N/A
// solenoid box 1: cannon, fountain, N/A, N/A
const boolean onSetting = true;
// also used
// D4, GPIO2, BUILTIN_LED

void setup() {
  // set them off, then enable pin.
  for( byte i=0; i<4; i++ ) {
    digitalWrite(pin[i], !onState);
    pinMode(pin[i], OUTPUT);
  }

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // configure comms
  Comms.begin("Water", processMessages);
  Comms.addSubscription(Comms.topicInteraction[0]);
  Comms.addSubscription(Comms.topicInteraction[1]);
  Comms.addSubscription(Comms.topicInteraction[2]);
  
  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void processMessages(String topic, String message) {

  // bail out if we're not processing our interaction
  if( ! topic.equals(topicInteraction) ) return;

  if( message.equals(Comms.messageInteraction[0]) ) { // idle
    
  }
}

