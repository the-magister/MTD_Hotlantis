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
#include <EL_driver.h>

// wire it up
// devices with the light shield have access to D5-D8
#define OUTPUT_PIN D5
#define ON HIGH
#define OFF LOW
// also used
// D4, GPIO2, BUILTIN_LED

void setup() {
  // for random numbers
  randomSeed(analogRead(0));

  // set them off, then enable pin.
  digitalWrite(OUTPUT_PIN, OFF); 
  pinMode(OUTPUT_PIN, OUTPUT);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( true ) {
    Comms.saveStuff("subTopic", Comms.actMTDFog[0]);
  }
  String subTopic = Comms.loadStuff("subTopic");
  Serial << "Startup: subscribing to [" << subTopic << "]." << endl;

  // configure comms
  Comms.begin(subTopic, processMessages);
  Comms.sub(subTopic); 

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void processMessages(String topic, String message) {

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]); // is on?
  if( setting ) setting = ON;
  else setting = OFF;
  
  digitalWrite(OUTPUT_PIN, setting);
}


