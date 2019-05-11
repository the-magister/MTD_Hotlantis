// IDE Settings:
// Tools->Board : "WeMos D1 R1" (!!!!!! NOTE !!!!!!)
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>

// wire it up
// devices with the light shield have access to D5-D8
// careful with D8: pulled up
#define IGNITER_PIN D7
#define ON HIGH
#define OFF LOW

// LED strips
#define LED_PIN D2
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip
#define NUM_LEDS 20
CRGBArray < NUM_LEDS > leds;

// EL devices
// pin 3, 5, 6, 9, 10, 11 are hardcoded on the shield
#define NUM_EL 6
byte ELPin[NUM_EL] = { D3, D5, D6, D9, D10, D11 };
// "/B" is D5 is extra LED on the board
// "/D" is D9 is pulled up and LED on SoC

void setup() {
  // set them off, then enable pin.
  digitalWrite(IGNITER_PIN, OFF);
  pinMode(IGNITER_PIN, OUTPUT);

  for (byte i = 0; i < NUM_EL; i++) {
    analogWrite(ELPin[i], 0);
    pinMode(ELPin[i], OUTPUT);
  }

  // wait a tic
  delay(250);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  Serial << F("Startup: configure leds...");
  FastLED.addLeds<WS2811, LED_PIN, COLOR_ORDER>(leds, leds.size()).setCorrection(COLOR_CORRECTION);
  FastLED.setBrightness(255);

  // bootstrap, if needed.
  if ( true ) {
    Comms.saveStuff("myName", "gwf/a/beacon/A");
  }
  String myName = Comms.loadStuff("myName");

  // configure comms
  Comms.begin(myName, processMessages, 99); // disable heartbeat LED
  Comms.sub(myName + "/#"); // all my messages

  Serial << F("Startup complete.") << endl;
}

// tracking traffic
unsigned long lastTrafficTime = millis();

void loop() {
  // comms handling
  Comms.loop();

  // are we bored yet?
  if ( (millis() - lastTrafficTime) > (10UL * 1000UL) ) {
    // show a throbbing rainbow background
    EVERY_N_MILLISECONDS(20) {
      static byte hue = 0;
      hue += 5;
      leds.fill_rainbow(hue, 255 / leds.size()); // paint
    }
    // cycle the EL segments
    EVERY_N_MILLISECONDS(20) {
      // https://github.com/FastLED/FastLED/wiki/High-performance-math#scaling
      for( int i=0; i<NUM_EL; i++ ) {
        int bpm = map(i, 0, NUM_EL-1, 15, 60); // bpm range
        int offset = 0 * i;
        analogWrite(ELPin[i], beatsin16(bpm, 0, PWMRANGE, 0, offset));
      }
    }
  }

  // should handle lights often, too.
  EVERY_N_MILLISECONDS(20) FastLED.show();
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // have update
  lastTrafficTime = millis();

  if ( topic.endsWith("igniter") ) {
    // what action?  on/off?
    boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF;
    digitalWrite(IGNITER_PIN, setting);
  } else if ( topic.endsWith("fire") ) {
    // what action?  on/off?
    boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF;
    // actually handled on the shore.  but, good to know.
  } else if ( topic.endsWith("light") ) {
    // LED lights
    // as a stupid example
    if ( message.equals("red") ) leds.fill_solid(CRGB::Red);
    else if ( message.equals("green") ) leds.fill_solid(CRGB::Green);
    else if ( message.equals("blue") ) leds.fill_solid(CRGB::Blue);
    else leds.fill_solid(CRGB::Black);
  } else {
    // EL lights
    // amount?
    int proportion = constrain(message.toInt(), 0, PWMRANGE);

    if ( topic.endsWith("A") ) analogWrite(ELPin[0], proportion);
    else if ( topic.endsWith("B") ) analogWrite(ELPin[1], proportion);
    else if ( topic.endsWith("C") ) analogWrite(ELPin[2], proportion);
    else if ( topic.endsWith("D") ) analogWrite(ELPin[3], PWMRANGE-proportion); // because pin is pulled up
    else if ( topic.endsWith("E") ) analogWrite(ELPin[4], proportion);
    else if ( topic.endsWith("F") ) analogWrite(ELPin[5], proportion);
  }
}
