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
#include <Ramp.h>

// Assumptions:

// 1. proportional control valve power is 24VDC, and the supply is controlled by a switch.

// 2. each tower has a proportional control valve:
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

void setPwmRange(int range) {
  range = constrain(range, 0, 65535);
  Serial << "Flame.  Setting PWM range to: 0-" << range << endl;
  analogWriteRange(range);
}
void setPwmFreq(int freq) {
  freq = constrain(freq, 0, 10000);
  Serial << "Flame.  Setting PWM frequency to: " << freq << endl;
  analogWriteFreq(freq);
}

// target for ramps
rampInt valveRamp[3];
unsigned long rampTime = 5000UL;
void setRampTime(unsigned long val) {
  rampTime = val;
  Serial << "Flame.  Setting ramp time to: " << rampTime << endl;
}
#define INTERP_MODE LINEAR
//#define LOOP_MODE ONCEFORWARD
#define LOOP_MODE FORTHANDBACK
void rampValves() {
//  float gamma = 0.5; // Correction factor
//  int corr = (int)(pow((float)valveRamp[0].update() / (float)1023, gamma) * 1023 + 0.5);
//  Serial <<  valveRamp[0].update() << " -> " << corr << endl;
//  delay(100);
  analogWrite(PROP_A_PIN, valveRamp[0].update());
  analogWrite(PROP_B_PIN, valveRamp[1].update());
  analogWrite(PROP_C_PIN, valveRamp[2].update());
}



void setup() {
  // set them off, then enable pin.
  analogWrite(PROP_A_PIN, 0); pinMode(PROP_A_PIN, OUTPUT);
  analogWrite(PROP_B_PIN, 0); pinMode(PROP_B_PIN, OUTPUT);
  analogWrite(PROP_C_PIN, 0); pinMode(PROP_C_PIN, OUTPUT);

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
  Comms.sub(Comms.actBeaconFlame[0]); // flame level A
  Comms.sub(Comms.actBeaconFlame[1]); // flame level B
  Comms.sub(Comms.actBeaconFlame[2]); // flame level C
  Comms.sub(Comms.actBeaconFlame[3]); // set pwmrange
  Comms.sub(Comms.actBeaconFlame[4]); // set pwmfreq
  Comms.sub(Comms.actBeaconFlame[5]); // set ramptime

  // configure proportional valves
  setPwmFreq(PROP_FREQ);
  setPwmRange(PWMRANGE); // 0..1023

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

  // flame handling
  rampValves();
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // amount?
  int val = constrain(message.toInt(), 0, 65535);

  // do it
  if ( topic.indexOf("/A/") != -1 ) valveRamp[0].go(val, rampTime, INTERP_MODE, LOOP_MODE);
  if ( topic.indexOf("/B/") != -1 ) valveRamp[1].go(val, rampTime, INTERP_MODE, LOOP_MODE);
  if ( topic.indexOf("/C/") != -1 ) valveRamp[2].go(val, rampTime, INTERP_MODE, LOOP_MODE);

  if ( topic.indexOf("pwmrange") != -1 ) setPwmRange(val); 
  if ( topic.indexOf("pwmfreq") != -1 ) setPwmFreq(val); 
  
  if ( topic.indexOf("ramptime") != -1 ) setRampTime((unsigned long)val);
}
