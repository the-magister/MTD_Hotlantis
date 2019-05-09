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

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>

// wire it up
// devices with the light shield have access to D5-D8
// If the relay(s) triggers LOW, use D3 and D4 as these are hardware pulled-up
// If the relay(s) trigger HIGH, use D8 as this is hardware pulled-down
#define IGNITER_PIN D1
#define FLAME_PIN D2
#define ON HIGH
#define OFF LOW
// also used
// D4, GPIO2, BUILTIN_LED

#define LED_PIN D5
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip
#define NUM_LEDS 20

CRGBArray < NUM_LEDS > leds;

void setup() {
  // set them off, then enable pin.
  digitalWrite(IGNITER_PIN, OFF); 
  pinMode(IGNITER_PIN, OUTPUT);
  digitalWrite(FLAME_PIN, OFF); 
  pinMode(FLAME_PIN, OUTPUT);
  
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  Serial << F("Startup: configure leds...");
  FastLED.addLeds<WS2811, LED_PIN, COLOR_ORDER>(leds, leds.size()).setCorrection(COLOR_CORRECTION);
  FastLED.setBrightness(255);
  
  // bootstrap, if needed.
  if ( true ) {
    Comms.saveStuff("flameTopic", Comms.actBeaconFlame[0]);
    Comms.saveStuff("lightTopic", Comms.actBeaconLight[0]);
    Comms.saveStuff("igniterTopic", Comms.actBeaconIgniter[0]);
  }
  String flameTopic = Comms.loadStuff("flameTopic");
  String lightTopic = Comms.loadStuff("lightTopic");
  String igniterTopic = Comms.loadStuff("igniterTopic");

  // configure comms
  String myName = lightTopic;
  myName.replace("/light","");
  Comms.begin(myName, processMessages);
  Comms.sub(flameTopic); 
  Comms.sub(lightTopic); 
  Comms.sub(igniterTopic); 

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// processes messages that arrive
void processMessages(String topic, String message) {

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF;

  if( topic.indexOf("igniter") != -1 ) {  
    digitalWrite(IGNITER_PIN, setting);
  } else if( topic.indexOf("fire") != -1 ) {  
    digitalWrite(FLAME_PIN, setting);
  } else {
    // Light
      // as a stupid example
      CRGB color;
      if( message.equals("red") ) color = CRGB::Red;
      if( message.equals("green") ) color = CRGB::Green;
      if( message.equals("blue") ) color = CRGB::Blue;
      
      leds.fill_solid(color);
      FastLED.show();
  }
}
