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

// TODO: These seem to be reversed on the hardware
// L POT is retrigger time.  CW slow.  CCW fast. set "fastest"
// R POT is sensitvity.  CW least.  CCW most.  set "mostest".

// topics
String motionTopic;
String lightTopic;

// track what we're doing with the lights.
String lightGesture = "rainbow";
boolean gestureChanged = false;
static int infinite = 60*60*60UL;

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

// color defs
// deck lighting
CRGB deckColor = CRGB::FairyLight;

// day of week pallette
CRGBPalette16 dowPalette;


void setup() {
  delay(250); // for reprogramming window
  
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // LEDs
  Serial << F("Startup: configure leds...");
  FastLED.addLeds<WS2811, D5, COLOR_ORDER>(leftFront, leftFront.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D6, COLOR_ORDER>(leftBack, leftBack.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D7, COLOR_ORDER>(rightBack, rightBack.size()).setCorrection(COLOR_CORRECTION);
  FastLED.addLeds<WS2811, D8, COLOR_ORDER>(rightFront, rightFront.size()).setCorrection(COLOR_CORRECTION);
  FastLED.setBrightness(255);
  Serial << F(" done.") << endl;

  // enable pin.
  motion.attach(PIN_MOTION, INPUT);
  motion.interval(5); // interval in ms

  // bootstrap, if needed.
  if ( false ) {
    Comms.saveStuff("motionTopic", Comms.senseMTDMotion[2]);
    Comms.saveStuff("lightTopic", Comms.actMTDLight[2]);
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
boolean off = false;  // . Off state means show no light ever

Metro gestureLockout(infinite);
boolean gestureLock = false;

void loop() {
  // comms handling
  Comms.loop();

  // update motion
  if ( motion.update() ) {
    boolean motionVal = motion.read();
    // publish new reading.
    Comms.pub(motionTopic, Comms.messageBinary[motionVal == onReading]);
    lastTrafficTime = millis();

    if (!off && motionVal == onReading) {
      CRGB color = CRGB::White;
      
      leftBar1.fill_solid(color);
      rightBar1.fill_solid(color);
      leftBar2.fill_solid(color);
      rightBar2.fill_solid(color);
      FastLED.show();

      gestureLock = true;
      gestureLockout.interval(400UL);
      gestureLockout.reset();
      return;
    }
  }

  if (gestureLockout.check()) {
    gestureLock = false;
    gestureLockout.interval(infinite);
  }
  
  //Up[date lights.  Off state means no lights
  if (off || gestureLock) return;

  // are we bored yet?
  if((millis() - lastTrafficTime) > (90UL * 1000UL) ) {
    lightGesture = "rainbow";
  }

  EVERY_N_MILLISECONDS(20) {

    if( lightGesture.equals("red") ) gestureSolidColor(CRGB::Red);
    else if( lightGesture.equals("green") ) gestureSolidColor(CRGB::Green);
    else if( lightGesture.equals("blue") ) gestureSolidColor(CRGB::Blue);
    else if( lightGesture.equals("black") ) gestureSolidColor(CRGB::Black);
    else if( lightGesture.equals("rainbow")) gestureRainbow();
    else if( lightGesture.equals("rainbowGlitter")) { gestureRainbow(); gestureGlitter(50); }
    else if( lightGesture.equals("confetti")) gestureConfetti();
    else if ( lightGesture == "sinelon" ) gestureSinelon();
    else if ( lightGesture == "BPM" ) gestureBPM();
    else if ( lightGesture == "juggle" ) gestureJuggle();
    else if ( lightGesture == "id" ) gestureIdentity();
    
    FastLED.show();
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // have update
  lastTrafficTime = millis();

  CRGB color;

  if( message.equals("off") ) {
    Serial << "Lighting turned off" << endl;
    off = true;
    gestureSolidColor(CRGB::Black);
    FastLED.show();
  } else {
    off = false;

    lightGesture = message;
    gestureChanged = true;
  }

}

void gestureSolidColor(CRGB color) {
  leftBack.fill_solid(color);
  rightBack.fill_solid(color);
  leftFront.fill_solid(color);
  rightFront.fill_solid(color);  
}

void gestureRainbow() {
  static byte hue = 0;
  hue += 5;
  
  leftBack.fill_rainbow(hue, 255 / leftBack.size()); // paint
  leftFront.fill_rainbow(hue + 128, -255 / leftFront.size());
  rightBack = leftBack;
  rightFront = leftFront;
}

void gestureGlitter(fract8 chanceOfGlitter) {
  if ( random8() < chanceOfGlitter) {
    leftFront[ random16(leftFront.size()) ] += CRGB::White;
  }
  if ( random8() < chanceOfGlitter) {
    leftBack[ random16(leftBack.size()) ] += CRGB::White;
  }
  if ( random8() < chanceOfGlitter) {
    rightFront[ random16(rightFront.size()) ] += CRGB::White;
  }
  if ( random8() < chanceOfGlitter) {
    rightBack[ random16(rightBack.size()) ] += CRGB::White;
  }
}

void gestureMotionHighlite() {
  fract8 chanceOfGlitter = 75;

  for(int i=0; i < 8; i++) {
    if ( random8() < chanceOfGlitter) {
      leftFront[ random16(leftFront.size()) ] += CRGB::White;
    }
    if ( random8() < chanceOfGlitter) {
      leftBack[ random16(leftBack.size()) ] += CRGB::White;
    }
    if ( random8() < chanceOfGlitter) {
      rightFront[ random16(rightFront.size()) ] += CRGB::White;
    }
    if ( random8() < chanceOfGlitter) {
      rightBack[ random16(rightBack.size()) ] += CRGB::White;
    }
  }
}

void gestureConfetti() {
  static byte hue = 0;
  // random colored speckles that blink in and fade smoothly
  leftBack.fadeToBlackBy(10);
  int pos = random16(leftBack.size());
  leftBack[pos] += CHSV( hue + random8(64), 200, 255);

  rightBack.fadeToBlackBy(10);
  pos = random16(rightBack.size());
  rightBack[pos] += CHSV( hue + random8(64), 200, 255);

  leftFront.fadeToBlackBy(10);
  pos = random16(leftFront.size());
  leftFront[pos] += CHSV( hue + random8(64), 200, 255);

  rightFront.fadeToBlackBy(10);
  pos = random16(rightFront.size());
  rightFront[pos] += CHSV( hue + random8(64), 200, 255);

  hue++;
}

void gestureSinelon() {
  static byte hue = 0;
  // a colored dot sweeping back and forth, with fading trails
  leftBack.fadeToBlackBy(20);
  int pos = beatsin16( 13, 0, leftBack.size() - 1 );
  leftBack[pos] += CHSV( hue, 255, 192);
  
  leftFront.fadeToBlackBy(20);
  pos = beatsin16( 13, 0, leftFront.size() - 1 );
  leftFront[pos] += CHSV( hue, 255, 192);

  rightBack.fadeToBlackBy(20);
  pos = beatsin16( 13, 0, rightBack.size() - 1 );
  rightBack[pos] += CHSV( hue, 255, 192);
  
  rightFront.fadeToBlackBy(20);
  pos = beatsin16( 13, 0, rightFront.size() - 1 );
  rightFront[pos] += CHSV( hue, 255, 192);

  hue++;

}
void gestureBPM() {
  static byte hue = 0;
  hue++;
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 12;
  static CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < leftBack.size(); i++) { //9948
    leftBack[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
  for ( int i = 0; i < rightBack.size(); i++) { //9948
    rightBack[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
  for ( int i = 0; i < leftFront.size(); i++) { //9948
    leftFront[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
  for ( int i = 0; i < rightFront.size(); i++) { //9948
    rightFront[i] = ColorFromPalette(palette, hue + (i * 2), beat - hue + (i * 10));
  }
}

void gestureJuggle() {
  // eight colored dots, weaving in and out of sync with each other
  leftBack.fadeToBlackBy(20);
  byte dothue = 0;
  byte n = 3;
  for ( int i = 0; i < n; i++) {
    leftBack[beatsin16( i + (n-1), 0, leftBack.size() - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }

  leftFront.fadeToBlackBy(20);
  dothue = 0;
  n = 3;
  for ( int i = 0; i < n; i++) {
    leftFront[beatsin16( i + (n-1), 0, leftFront.size() - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }

  rightBack.fadeToBlackBy(20);
  dothue = 0;
  n = 3;
  for ( int i = 0; i < n; i++) {
    rightBack[beatsin16( i + (n-1), 0, rightBack.size() - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }

  rightFront.fadeToBlackBy(20);
  dothue = 0;
  n = 3;
  for ( int i = 0; i < n; i++) {
    rightFront[beatsin16( i + (n-1), 0, rightFront.size() - 1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// Identity each strip
void gestureIdentity() {
  leftBack.fill_solid(CRGB::Red);
  rightBack.fill_solid(CRGB::Green);
  leftFront.fill_solid(CRGB::Blue);
  rightFront.fill_solid(CRGB::White);  
}

// upstairs is always chill.  Need to be able to see the stairs.
void updateDeck() {
  leftDeck.fill_solid(deckColor);
  leftDeck.fadeLightBy( 128 ); // dim it

  rightDeck = leftDeck;
}
