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
// careful with D3 and D8: pulled up
// careful with D8: pulled up
#define IGNITER_PIN D8
#define ON HIGH
#define OFF LOW

// our internal storage, mapped to the hardware.
// pay no attention to the man behind the curtain.
#define COLOR_ORDER RGB
#define COLOR_CORRECTION TypicalLEDStrip
#define NUM_SEGMENTS 3 // D5, D6, D7
// three vertical columns of lights.
// facing the panels, left and right are sparse (n=10) and 
// the middle 
#define LEDS_LEFT_COL 8
#define LEDS_CENTER_COL 15
#define LEDS_RIGHT_COL 8
// total
#define LEDS_PANEL LEDS_LEFT_COL + LEDS_CENTER_COL + LEDS_RIGHT_COL

// symmetric panels, A->B->C in clockwise direction
CRGBArray < LEDS_PANEL > ledA;
CRGBArray < LEDS_PANEL > ledB;
CRGBArray < LEDS_PANEL > ledC;

// subsets of each panel
CRGBSet leftA = ledA(0, LEDS_LEFT_COL - 1);
CRGBSet leftB = ledB(0, LEDS_LEFT_COL - 1);
CRGBSet leftC = ledC(0, LEDS_LEFT_COL - 1);
//
CRGBSet centerAr = ledA(LEDS_LEFT_COL, LEDS_LEFT_COL + LEDS_CENTER_COL - 1); // installed reversed
CRGBSet centerBr = ledB(LEDS_LEFT_COL, LEDS_LEFT_COL + LEDS_CENTER_COL - 1); // installed reversed
CRGBSet centerCr = ledC(LEDS_LEFT_COL, LEDS_LEFT_COL + LEDS_CENTER_COL - 1); // installed reversed
CRGBSet centerA = centerAr(centerAr.size()-1, 0);
CRGBSet centerB = centerAr(centerBr.size()-1, 0);
CRGBSet centerC = centerAr(centerCr.size()-1, 0);
//
CRGBSet rightA = ledA(LEDS_LEFT_COL + LEDS_CENTER_COL, LEDS_LEFT_COL + LEDS_CENTER_COL + LEDS_RIGHT_COL - 1);
CRGBSet rightB = ledB(LEDS_LEFT_COL + LEDS_CENTER_COL, LEDS_LEFT_COL + LEDS_CENTER_COL + LEDS_RIGHT_COL - 1);
CRGBSet rightC = ledC(LEDS_LEFT_COL + LEDS_CENTER_COL, LEDS_LEFT_COL + LEDS_CENTER_COL + LEDS_RIGHT_COL - 1);

// track what we're doing with the lights.
String lightGesture = "BPM";

void setup() {
  // set them off, then enable pin.
  digitalWrite(IGNITER_PIN, OFF);
  pinMode(IGNITER_PIN, OUTPUT);

  // wait a tic
  delay(250);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // LEDs
  Serial << F("Startup: configure leds...");
  FastLED.addLeds<WS2811, D5, COLOR_ORDER>(ledA, ledA.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D6, COLOR_ORDER>(ledB, ledB.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D7, COLOR_ORDER>(ledC, ledC.size()).setCorrection(COLOR_CORRECTION);
  FastLED.setBrightness(255);
  Serial << F(" done.") << endl;

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
    lightGesture = "BPM";
  }

  // set a reasonable frame rate so the WiFi has plenty of cycles.
  EVERY_N_MILLISECONDS(20) {
    if ( lightGesture == "rainbow" ) {
      gestureRainbow();
    } else if ( lightGesture == "rainbowGlitter" ) {
      gestureRainbow();
      gestureGlitter(50);
    } else if ( lightGesture == "confetti" ) gestureConfetti(); 
    else if ( lightGesture == "sinelon" ) gestureSinelon();
    else if ( lightGesture == "BPM" ) gestureBPM();
    else if ( lightGesture == "juggle" ) gestureJuggle();
    else if ( lightGesture == "red" ) ledA.fill_solid(CRGB::Red);
    else if ( lightGesture == "green" ) ledA.fill_solid(CRGB::Green);
    else if ( lightGesture == "blue" ) ledA.fill_solid(CRGB::Blue);
    else ledA.fill_solid(CRGB::White);

    ledC = ledB = ledA;
    FastLED.show();
  }
}

// lifted from Examples->FastLED->DemoRee100
void gestureRainbow() {
  static byte hue = 0;
  hue += 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_rainbow(hue, LEDS_CENTER_COL); 
  rightA = leftA;  
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); 

//  ledA.fill_rainbow(hue, 255 / ledA.size()); // paint
}
void gestureGlitter(fract8 chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    ledA[ random16(ledA.size()) ] += CRGB::White;
  }
}
void gestureConfetti() {
  static byte hue = 0;
  // random colored speckles that blink in and fade smoothly
  ledA.fadeToBlackBy(10);
  int pos = random16(ledA.size());
  ledA[pos] += CHSV( hue++ + random8(64), 200, 255);
}
void gestureSinelon() {
  static byte hue = 0;
  // a colored dot sweeping back and forth, with fading trails
  ledA.fadeToBlackBy(20);
  int pos = beatsin16( 13, 0, ledA.size() - 1 );
  ledA[pos] += CHSV( hue++, 255, 192);
}
void gestureBPM() {
  static byte hue = 0;
  hue++;
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 12;
  static CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < ledA.size(); i++) { //9948
    ledA[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
}
void gestureJuggle() {
  // eight colored dots, weaving in and out of sync with each other
  ledA.fadeToBlackBy(20);
  byte dothue = 0;
  byte n = 3;
  for ( int i = 0; i < n; i++) {
    ledA[beatsin16( i + (n-1), 0, ledA.size() - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
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
    // LED lights; save it
    lightGesture = message;
    // as a stupid example
  }
}
