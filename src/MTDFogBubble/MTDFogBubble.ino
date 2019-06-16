// Compile for Uno.

// The IDE requires all libraries to be #include’d in the main (.ino) file.  Clutter.
#include <Streaming.h> // <<-style printing
#include <Metro.h> // countdown timers

// handle incoming instructions and idle patterns
// perform lighting
#include <DmxSimple.h>

// pin locations for outputs
#define PIN_DMX 3 // the pin that drives the DMX system
#define PIN_LED BUILTIN_LED

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Serial << F("Setup: begin") << endl;

  Serial << F("Setup: pausing after boot 1 sec...") << endl;
  delay(1000UL);

  // startup
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_DMX, OUTPUT);
  DmxSimple.usePin(PIN_DMX);
   
  Serial << F("Setup: complete") << endl;
}

void loop() {
 

}

