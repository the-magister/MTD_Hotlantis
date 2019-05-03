// IDE Settings:
// Tools->Board : "Adafruit Feather HUZZAH ESP8266"
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
const boolean onReading = false;
// devices with the light shield block access to D5-D8
Bounce button[2];
// wire GPIO14 (left) and GPIO12 (right) through a N.O. switch to GND
#define LEFT_PIN 14
#define RIGHT_PIN 12
const byte pin[2] = {LEFT_PIN, RIGHT_PIN}; 

void setup() {
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << F("Startup begins.") << endl;

  // set up pins
  for( byte i=0; i<sizeof(pin); i++ ) {
    button[i].attach(pin[i], INPUT_PULLUP); // so, a reading of 1 is "not pressed"
    button[i].interval(5); 
  }
  
  // start
  wakeModem();

  Serial << F("Startup complete.") << endl;
}

// what are we doing?
boolean systemActive = false;

void wakeModem() {
  Serial << "Modem WAKE." << endl;
  systemActive = true;
  
  Comms.begin("Cannon", processMessages); delay(1);
}

void sleepModem() {
  Serial << "Modem SLEEP." << endl;
  systemActive = false;
  
  Comms.end(); delay(1);
  WiFi.mode(WIFI_OFF); delay(1);
  //For some reason the modem won't go to sleep unless you do a delay(non-zero-number) -- no delay, no sleep and delay(0), no sleep
}

void loop() {
  // modem sleep handling
  static Metro sleepDelay(1000UL * 60UL * 5UL); // turn off modem after 5 minutes of inactivity
  if ( sleepDelay.check() && systemActive ) sleepModem();

  // comms handling
  if ( systemActive ) Comms.loop();

  // update buttons
  for( byte i=0; i<sizeof(pin); i++ ) {
    if( button[i].update() ) {
      // wake up, if needed.
      if ( ! systemActive ) wakeModem();
  
      // note activity and prevent sleep
      sleepDelay.reset();
  
      // publish new reading.
      Comms.publish(Comms.topicCannon[i], Comms.messageBinary[button[i].read() == onReading]); 
    }
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {
}

