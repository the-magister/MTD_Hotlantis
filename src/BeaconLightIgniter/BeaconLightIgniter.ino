/*
   IDE Settings:
   Tools->Board : "WeMos D1 R2 & mini"
   Tools->Flash Size : "4M (1M SPIFFS)"
   Tools->CPU Frequency : "160 MHz"
   Tools->Upload Speed : "921600"
*/

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>
#include <ArduinoJson.h>

#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>

// wire it up
// devices with the light shield have access to D5-D8
// careful with D3 and D8: pulled up
// careful with D8: pulled up
#define IGNITER_PIN D1
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

#define sideVal 150  // side values when center is 255

// track what we're doing with the lights.
String lightGesture = "blue";
byte hue1;
byte hue2;
byte hue3;
boolean gestureChanged = false;

boolean randPicked = false;
int lastLen = 0;

// track we we're doing with the igniter
boolean igniterState = OFF;
Metro igniterMaxTimer(60UL * 1000UL);  

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
//  Comms.saveStuff("myName", "gwf/a/beacon/C");
  String myName = Comms.loadStuff("myName");

  if (myName.length() == 0) {
    myName = "gwf/a/beacon/T";  // Default for local debugging
  }

  // configure comms
  Comms.begin(myName, processMessages); 
  // set QoS=1. We are at the longest range and we control a stun gun,
  // we'd like to get commands reliably.
  Comms.setMQTTQOS(1);
  
  // subs  
  Comms.sub(myName + "/#"); // all my messages

  randomSeed(42);  // Use the same random number in hopes that beacons will be locked in random choices
  Serial << F("Startup complete.") << endl;
}

// tracking traffic
unsigned long lastTrafficTime = millis();

void loop() {
  // comms handling
  Comms.loop();

  if (igniterState == ON) {
    if (igniterMaxTimer.check()) {
      Serial << "Max igniter time exceeded.  Turning off igniter" << endl;
      digitalWrite(IGNITER_PIN, OFF);
      igniterState = OFF;
      return;
    }
    onetimePrint("Igniter active, ignoring light");
    return;
  }
  
  // are we bored yet?
  if ( (millis() - lastTrafficTime) > (30UL * 1000UL) ) {
    lightGesture = "centerRainbowSideRand";

    onetimePrint("Bored.  Switching to: " + lightGesture);
  }

  // set a reasonable frame rate so the WiFi has plenty of cycles.
  EVERY_N_MILLISECONDS(20) {
    if (gestureChanged) {
      randPicked = false;
    }

    // Direct control first as we are moving to this
    if ( lightGesture == "centerHue1") gestureCenterHue1();
    else if ( lightGesture == "centerHue1SideHue2") gestureCenterHue1SideHue2();
    else if ( lightGesture == "centerRainbowSideHue1") gestureCenterRainbowSideHue1();        
    else if ( lightGesture == "solidHue1") gestureSolidHue1();        
    else if ( lightGesture == "rainbow" ) {
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
    else if ( lightGesture == "maxWhite" ) ledA.fill_solid(CRGB::White);
    else if ( lightGesture == "centerRandSideRand") gestureCenterRandSideRand();
    else if ( lightGesture == "centerRandSideAnalog") gestureCenterRandSideAnalog();
    else if ( lightGesture == "centerRandSideComp") gestureCenterRandSideComp();
    else if ( lightGesture == "centerRainbowSideRand") gestureCenterRainbowSideRand();    
    else if ( lightGesture == "centerRainbowSideBlack") gestureCenterRainbowSide(CRGB::Black);
    else if ( lightGesture == "centerRainbowSideRainbow") gestureCenterRainbowSideRainbow();
    else if ( lightGesture == "centerRainbowSideRainbowOpp") gestureCenterRainbowSideRainbowOpp();
    else if ( lightGesture == "centerSinelonSideSinelon") gestureCenterSinelonSideSinelon();
    else if ( lightGesture == "centerSinelonSideSinelonOpp") gestureCenterSinelonSideSinelonOpp();
    else if ( lightGesture == "black") ledA.fill_solid(CRGB::Black);
    else if ( lightGesture == "random") {
      static int randomIdx = 0;
      if (gestureChanged) {
        randomIdx = random(0,11);  
        onetimePrint("Random Pattern choosen: " + String(randomIdx));
      }
      
      switch(randomIdx) {
        case 0: gestureRainbow(); break;
        case 1: gestureRainbow(); gestureGlitter(50); break;
        case 2: gestureConfetti(); break;
        case 3: gestureSinelon(); break;
        case 4: gestureBPM(); break;
        case 5: gestureJuggle(); break;
        case 6: gestureCenterRainbowSideRainbow(); break;
        case 7: gestureCenterRandSideRand(); break;  // Perhaps remove if the analog and comp look nicer
        case 8: gestureCenterRainbowSideRand(); break;
        case 9: gestureCenterRandSideAnalog(); break;
        case 10: gestureCenterRandSideComp(); break;
      }
    }
    else {
      onetimePrint("Unknown gesture: " + lightGesture);
      ledA.fill_solid(CRGB::Red);
    }

    ledC = ledB = ledA;  // Copy animation to each face
    FastLED.show();

    gestureChanged = false;
  }
} 

void gestureSolidHue1() {
  CHSV color = CHSV(hue1,255,255);
  CRGB colorRgb = CRGB(color);
  ledA.fill_solid(colorRgb);
}

void gestureCenter(CRGB color) {
   ledA.fill_solid(CRGB::Black); centerA.fill_solid(color);
}
void gestureCenterHue1() {
  CHSV center = CHSV(hue1,255,255);
  CRGB centerRgb = CRGB(center);
  
  ledA.fill_solid(CRGB::Black); centerA.fill_solid(centerRgb);
}
void gestureCenterSide(CRGB color, CRGB side) {
   ledA.fill_solid(CRGB::Black); 
   leftA.fill_solid(side);
   centerA.fill_solid(color);
   rightA.fill_solid(side);
}

// Pick a random center color, then an analog side color(+- 32 hue in fastled)
void gestureCenterRandSideAnalog() {
  ledA.fill_solid(CRGB::Black); 

  static byte chue = 160;
  if (!randPicked) {
    chue = random(0,255);
  }

  CHSV center = CHSV(chue, 255, 255);
  CRGB centerRgb = CRGB(center);
  static byte shue = 0;

  if (!randPicked) {
    byte delta = random(0,2);
    if (delta == 0) delta = -32;
    else delta = 32;
    
    shue = chue + delta;     
    randPicked = true;

    Serial << "Center: " << chue << " side: " << shue << endl;
  }

  CHSV side = CHSV( shue, 255, sideVal);  // Doesnt work with negative dir stuff, so convert to RGB
  CRGB sideRgb = CRGB(side);

  leftA.fill_solid(sideRgb);
  centerA.fill_solid(centerRgb);
  rightA.fill_solid(sideRgb);
}

// Pick a random center color, then its complementary color
void gestureCenterRandSideComp() {
  ledA.fill_solid(CRGB::Black); 

  static byte chue = 160;
  if (!randPicked) {
    chue = random(0,255);
  }

  CHSV center = CHSV(chue, 255, 255);
  CRGB centerRgb = CRGB(center);
  static byte shue = 0;

  if (!randPicked) {
    shue = -chue;
    randPicked = true;

    Serial << "Center: " << chue << " side: " << shue << endl;
  }

  CHSV side = CHSV( shue, 255, sideVal);  // Doesnt work with negative dir stuff, so convert to RGB
  CRGB sideRgb = CRGB(side);

  leftA.fill_solid(sideRgb);
  centerA.fill_solid(centerRgb);
  rightA.fill_solid(sideRgb);
}

void gestureCenterHue1SideHue2() {
  ledA.fill_solid(CRGB::Black); 

  CHSV side = CHSV(hue2, 255, sideVal);  // Doesnt work with negative dir stuff, so convert to RGB
  CRGB sideRgb = CRGB(side);

  CHSV center = CHSV(hue1, 255, 255);
  CRGB centerRgb = CRGB(center);
     
  leftA.fill_solid(sideRgb);
  centerA.fill_solid(centerRgb);
  rightA.fill_solid(sideRgb);
}

void gestureCenterRandSideRand() {
  ledA.fill_solid(CRGB::Black); 

  static byte shue = 0;
  if (!randPicked) {
    shue = random(0,255);   
    Serial << " New shue: " << shue << endl;
  }

  CHSV side = CHSV( shue, 255, sideVal);  // Doesnt work with negative dir stuff, so convert to RGB
  CRGB sideRgb = CRGB(side);

  static byte chue = 160;
  if (!randPicked) {
    randPicked = true;
    chue = random(0,255);
  }

   CHSV center = CHSV(chue, 255, 255);
   CRGB centerRgb = CRGB(center);
   
   leftA.fill_solid(sideRgb);
   centerA.fill_solid(centerRgb);
   rightA.fill_solid(sideRgb);
}

void gestureCenterRainbowSideHue1() {  
  CHSV side = CHSV(hue1,255,sideVal);
  CRGB sideRgb = CRGB(side);
  
  static byte hue = 0;
  hue += 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_solid(sideRgb);
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); // should be centerA.size() not the hardcode value
  rightA.fill_solid(sideRgb);
}

void gestureCenterRainbowSideRand() {
  static byte shue = 160;
  if (!randPicked) {
    randPicked = true;
    shue = random(0,255);
  }
  CHSV side = CHSV(shue,255,sideVal);
  CRGB sideRgb = CRGB(side);
  
  static byte hue = 0;
  hue += 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_solid(sideRgb);
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); // should be centerA.size() not the hardcode value
  rightA.fill_solid(sideRgb);
}

void gestureCenterRainbowSide(CRGB side) {
  static byte hue = 0;
  hue += 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_solid(side);
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
  rightA.fill_solid(side);
}

void gestureCenterRainbowSideRainbow() {
  static byte hue = 0;
  hue += 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_rainbow(hue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
  rightA.fill_rainbow(hue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
}
void gestureCenterRainbowSideRainbowOpp() {
  static byte hue = 0;
  static byte shue = 255;
  hue += 5;
  shue -= 5;
  // per-module step size through color wheel proportional to the 
  leftA.fill_rainbow(shue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
  centerA.fill_rainbow(hue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.
  rightA.fill_rainbow(shue, LEDS_LEFT_COL); // the second parameter should be the hue increment per pixel.  Odd choice.

  // I think you want to increment the leftA and rightA by a larger number than centerA, so you end up spanning the same distance around the color wheel?
}

void gestureCenterSinelonSideSinelon() {
  static byte hue = 0;
  // a colored dot sweeping back and forth, with fading trails
  ledA.fadeToBlackBy(20);
  int pos = beatsin16( 13, 0, leftA.size() - 1 );
  leftA[pos] += CHSV( hue, 255, sideVal);
  rightA[pos] += CHSV( hue, 255, sideVal);
  
  int pos2 = beatsin16( 13, 0, centerA.size() - 1 );
  centerA[pos2] += CHSV( hue++, 255, 192);
}

void gestureCenterSinelonSideSinelonOpp() {
  static byte hue = 0;

  // a colored dot sweeping back and forth, with fading trails
  ledA.fadeToBlackBy(20);
  int pos = beatsin16( 13, 0, leftA.size() - 1 );
  leftA[pos] += CHSV( hue, 255, sideVal);
  rightA[pos] += CHSV( hue, 255, sideVal);
  
  int pos2 = beatsin16( 13, 0, centerA.size() - 1 );
  centerA[centerA.size()-pos2] += CHSV( hue++, 255, 192);
}

// lifted from Examples->FastLED->DemoReel100
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

void forceAllBlack() {
    ledA.fill_solid(CRGB::Black);
    ledB.fill_solid(CRGB::Black);
    ledC.fill_solid(CRGB::Black);
}

// Print something once.  Use length as a marker for change, not perfect but I don't know of something better
void onetimePrint(String msg) {
  
  if (msg.length() != lastLen) {
    Serial << msg << endl;
    lastLen = msg.length();
  }
}

StaticJsonDocument<200> doc;

// processes messages that arrive
void processMessages(String topic, String message) {
  // have update
  lastTrafficTime = millis();

  if ( topic.endsWith("igniter") ) {
    // what action?  on/off?
    igniterState = message.equals(Comms.messageBinary[1]) ? ON : OFF;
    forceAllBlack();
    FastLED.show();
    delay(20);

    digitalWrite(IGNITER_PIN, igniterState);
    igniterMaxTimer.reset();
  } else if ( topic.endsWith("fire") ) {
    // what action?  on/off?
    boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF;
    // actually handled on the shore.  but, good to know.
  } else if ( topic.endsWith("light") ) {
    Serial << "Got light message: " << message << endl;
    if (message.startsWith("{")) {
      //hue mapping:    0 red,32 orange,64 yellow,96 green,128 aqua,160 blue,192 purple,224 pink
      // example gwf/a/beacon/A/light={g:"centerRainbowSideHue1",h1:127}
      DeserializationError error = deserializeJson(doc, message);
    
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }
      const char* sensor = doc["g"];
      lightGesture = String(sensor);
      hue1 = doc["h1"];
      hue2 = doc["h2"];
      hue3 = doc["h3"];

      Serial << "Sensor: " << lightGesture << " Hue1: " << hue1 << " Hue2: " << hue2 << " Hue3: " << hue3 << endl;
    } else {
      lightGesture = message;
    }
    gestureChanged = true;
    lastLen = 0;  // force a print always
  }
}
