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

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>

// spammy?
#define SHOW_SERIAL_DEBUG true

// wire it up
const boolean onReading = true;
// devices with the light shield block access to D5-D8
#define PIN_BUTTON D3 // wire D3/GPIO0 to PIR; +3.3 with trigger; GND no trigger.
Bounce motion = Bounce();
String motionTopic;

String interactionTopic;

// who am i?
byte myArch;
byte leftArch, rightArch;
byte leftCoord, rightCoord;

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

// color choices, based on arch information
const CHSV archColor[3] = {
  CHSV(HUE_RED, 255, 255),
  CHSV(HUE_GREEN, 255, 255),
  CHSV(HUE_BLUE, 255, 255)
};

// deck lighting
CRGB deckColor = CRGB::FairyLight;

void setup() {
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
  motion.attach(PIN_BUTTON, INPUT);
  motion.interval(5); // interval in ms

  // bootstrap, if needed.
  if ( false ) {
    Comms.saveTopic("motionTopic", Comms.topicMotion[0]);
    Comms.saveTopic("interactionTopic", Comms.topicInteraction[0]);
  }
  motionTopic = Comms.loadTopic("motionTopic");
  Serial << "Startup: publishing to topic [" << motionTopic << "]." << endl;
  interactionTopic = Comms.loadTopic("interactionTopic");
  Serial << "Startup: subscribing to topic [" << interactionTopic << "]." << endl;

  if ( interactionTopic.equals(Comms.topicInteraction[0]) ) myArch = 0;
  else if ( interactionTopic.equals(Comms.topicInteraction[1]) ) myArch = 1;
  else if ( interactionTopic.equals(Comms.topicInteraction[2]) ) myArch = 2;
  
  switch (myArch) {
    case 0:
      // sC.isPlayer indexes
      leftArch = 1; rightArch = 2;
      // sC.areCoordinated indexes
      leftCoord = 0; rightCoord = 2;
      break;
    case 1:
      leftArch = 2; rightArch = 0;
      leftCoord = 1; rightCoord = 0;
      break;
    case 2:
      leftArch = 0; rightArch = 1;
      leftCoord = 2; rightCoord = 1;
      break;
  }
  Serial << F("Arch indexes: left=") << leftArch << F(" my=") << myArch << F(" right=") << rightArch << endl;
  Serial << F("Coordination indexes: left=") << leftCoord << F(" right=") << rightCoord << endl;

  // configure comms
  Comms.begin("Motion_Light", processMessages);
  Comms.addSubscription(interactionTopic);

  Serial << "Startup: complete." << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // update motion
  if ( motion.update() ) {
    // publish new reading.
    Comms.publish(motionTopic, Comms.messageBinary[motion.read() == onReading]);
  }
}


// processes messages that arrive
void processMessages(String topic, String message) {
}
