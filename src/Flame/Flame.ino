// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

// Assumptions:

// 1. propane has a bypass that constantly flows for pilot activity.

// 2. proportional control valve power is 24VDC, and the supply is controlled by a switch.

// 3. each tower has a proportional control valve:
//    SD8202G004V: 35 psi max, 1/4" NPT, 0.50 Cv
//    We control these by a MOSFET and PWM.
#define PROP_A_PIN D5
#define PROP_B_PIN D6
#define PROP_C_PIN D7
// 50 works
//#define PROP_FREQ 50 // Hz; ASCO recommends 300 Hz for Air/Gas
// 10 lets you go low, but "chugs"
//#define PROP_FREQ 10 // Hz; ASCO recommends 300 Hz for Air/Gas
// 100 is nice,  floor is ~500.
//#define PROP_FREQ 100 // Hz; ASCO recommends 300 Hz for Air/Gas
// 60 gets down to ~350 floor.
#define PROP_FREQ 60 // Hz; ASCO recommends 300 Hz for Air/Gas

// 4. have button to send igniter signal
#define IGNITER_PIN D1 // wire to GND
#define IGNITER_ON LOW
#define IGNITER_OFF HIGH
Bounce igniterButton = Bounce();

// also used
// D4, GPIO2, BUILTIN_LED

// automagically shutdown
bool flameOn = false;
Metro flameAutoShutdown(30UL * 1000UL);

void setup() {
  // set them off, then enable pin.
  analogWrite(PROP_A_PIN, 0); pinMode(PROP_A_PIN, OUTPUT);
  analogWrite(PROP_B_PIN, 0); pinMode(PROP_B_PIN, OUTPUT);
  analogWrite(PROP_C_PIN, 0); pinMode(PROP_C_PIN, OUTPUT);
  analogWriteFreq(PROP_FREQ);

  // wait a tic
  delay(250);

  // enable pin.
  igniterButton.attach(IGNITER_PIN, INPUT_PULLUP);
  igniterButton.interval(5); // interval in ms

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // configure comms
  Comms.begin("Flame", processMessages);
  Comms.sub(Comms.actBeaconFlame[0]);
  Comms.sub(Comms.actBeaconFlame[1]);
  Comms.sub(Comms.actBeaconFlame[2]);

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // update button
  if ( igniterButton.update() ) {
    // publish new reading.
    Comms.pub(Comms.actBeaconIgniter[0], Comms.messageBinary[igniterButton.read() == IGNITER_ON]);
    Comms.pub(Comms.actBeaconIgniter[1], Comms.messageBinary[igniterButton.read() == IGNITER_ON]);
    Comms.pub(Comms.actBeaconIgniter[2], Comms.messageBinary[igniterButton.read() == IGNITER_ON]);
  }

  // maybe shutdown?
  static boolean doAutoShutdown = false;
  if ( doAutoShutdown && flameOn && flameAutoShutdown.check() ) {
    Serial << "Bad! Shutting down flame automatically." << endl;
    analogWrite(PROP_A_PIN, 0);
    analogWrite(PROP_B_PIN, 0);
    analogWrite(PROP_C_PIN, 0);
    flameOn = false;
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // amount?
  int proportion = constrain(message.toInt(), 0, PWMRANGE);

  // set an automatic timer to shutdown
  if ( proportion > 0 ) {
    flameOn = true;
    flameAutoShutdown.reset();
  }

  // do it
  if ( topic.indexOf("/A/") != -1 ) analogWrite(PROP_A_PIN, proportion);
  if ( topic.indexOf("/B/") != -1 ) analogWrite(PROP_B_PIN, proportion);
  if ( topic.indexOf("/C/") != -1 ) analogWrite(PROP_C_PIN, proportion);
}
