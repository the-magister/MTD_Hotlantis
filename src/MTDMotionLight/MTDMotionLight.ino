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

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>

// spammy?
#define SHOW_SERIAL_DEBUG true

// wire it up
const boolean onReading = true;
// devices with the light shield block access to D5-D8
#define PIN_MOTION D1 // wire D3/GPIO0 to PIR; +3.3 with trigger; GND no trigger.
Bounce motion = Bounce();
// L POT is retrigger time.  CW slow.  CCW fast. set "fastest"
// R POT is sensitvity.  CW least.  CCW most.  set "mostest"

// topics
String motionTopic;
String lightTopic;

// our internal storage, mapped to the hardware.
// pay no attention to the man behind the curtain.
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip
#define NUM_PINS 4
#define LEDS_BAR 3
#define LEDS_DOWN 17
#define LEDS_UP 13
#define LEDS_DECK 4

CRGBArray < LEDS_BAR + LEDS_DOWN > leftBack;
CRGBArray < LEDS_BAR + LEDS_DOWN > rightBack;
CRGBArray < LEDS_BAR + LEDS_UP + LEDS_DECK > leftFront;
CRGBArray < LEDS_BAR + LEDS_UP + LEDS_DECK > rightFront;

// bars
CRGBSet rightBar1 = rightBack(0, LEDS_BAR - 1);
CRGBSet rightBar2 = rightFront(0, LEDS_BAR - 1);
CRGBSet leftBar1 = leftBack(0, LEDS_BAR - 1);
CRGBSet leftBar2 = leftFront(0, LEDS_BAR - 1);

// verticals
CRGBSet leftUp = leftFront(LEDS_BAR, LEDS_BAR + LEDS_UP - 1);
CRGBSet rightUp = rightFront(LEDS_BAR, LEDS_BAR + LEDS_UP - 1);
CRGBSet leftDown = leftBack(LEDS_BAR, LEDS_BAR + LEDS_DOWN - 1);
CRGBSet rightDown = rightBack(LEDS_BAR, LEDS_BAR + LEDS_DOWN - 1);

// deck lights under rail
CRGBSet leftDeck = leftFront(LEDS_BAR + LEDS_UP, LEDS_BAR + LEDS_UP + LEDS_DECK - 1);
CRGBSet rightDeck = rightFront(LEDS_BAR + LEDS_UP, LEDS_BAR + LEDS_UP + LEDS_DECK - 1);

void setup() {
  delay(250); // for reprogramming window
  
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // LEDs
  Serial << F("Startup: configure leds...");
  FastLED.addLeds<WS2811, D5, COLOR_ORDER>(leftBack, leftBack.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D6, COLOR_ORDER>(rightBack, rightBack.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D7, COLOR_ORDER>(leftFront, leftFront.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D8, COLOR_ORDER>(rightFront, rightFront.size()).setCorrection(COLOR_CORRECTION);
  FastLED.setBrightness(255);
  Serial << F(" done.") << endl;

  // enable pin.
  motion.attach(PIN_MOTION, INPUT);
  motion.interval(5); // interval in ms

  // bootstrap, if needed.
  if ( true ) {
    Comms.saveStuff("motionTopic", Comms.senseMTDMotion[0]);
    Comms.saveStuff("lightTopic", Comms.actMTDLight[0]);
  }
  motionTopic = Comms.loadStuff("motionTopic");
  Serial << "Startup: publishing to topic [" << motionTopic << "]." << endl;
  lightTopic = Comms.loadStuff("lightTopic");
  Serial << "Startup: subscribing to topic [" << lightTopic << "]." << endl;

  // configure comms
  String myName = lightTopic;
  myName.replace("/light","");
  Comms.begin(myName, processMessages);
  Comms.sub(lightTopic);

  Serial << "Startup: complete." << endl;
}

// tracking traffic
unsigned long lastTrafficTime = millis();

void loop() {
  // comms handling
  Comms.loop();

  // update motion
  if ( motion.update() ) {
    // publish new reading.
    Comms.pub(motionTopic, Comms.messageBinary[motion.read() == onReading]);

    CRGB color = motion.read() == onReading ? CRGB::White : CRGB::Black;
    
    leftBack.fill_solid(color);
    rightBack.fill_solid(color);
    leftFront.fill_solid(color);
    rightFront.fill_solid(color);
    FastLED.show();

    lastTrafficTime = millis();
  }

  // are we bored yet?
  if( (millis() - lastTrafficTime) > (10UL * 1000UL) ) {
    // show a throbbing rainbow background
    EVERY_N_MILLISECONDS(20) {
      static byte hue = 0;
      hue += 5;
      
      leftBack.fill_rainbow(hue, 255 / leftBack.size()); // paint
      leftFront.fill_rainbow(hue + 128, -255 / leftFront.size());
      rightBack = leftBack;
      rightFront = leftFront;
    }
  }

  // should handle lights often, too.
  EVERY_N_MILLISECONDS(20) FastLED.show();
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // have update
  lastTrafficTime = millis();

  // as a stupid example
  CRGB color;
  if( message.equals("red") ) color = CRGB::Red;
  if( message.equals("green") ) color = CRGB::Green;
  if( message.equals("blue") ) color = CRGB::Blue;

  leftBack.fill_solid(color);
  rightBack.fill_solid(color);
  leftFront.fill_solid(color);
  rightFront.fill_solid(color);
}
