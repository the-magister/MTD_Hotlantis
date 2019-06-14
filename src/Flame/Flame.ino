// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

/* for a nice demo:
 *  find a level that's reliably 'just on"
 *  gwf/a/beacon/A/fire=350
 *  gwf/a/beacon/ramptime=5000
 *  gwf/a/beacon/rampmode=3 
 *  gwf/a/beacon/ramploop=2
 *  gwf/a/beacon/A/fire=1023
 */

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
// #define PROP_FREQ 60 // Hz; ASCO recommends 300 Hz for Air/Gas

// 3. have button to send igniter signal
#define IGNITER_PIN D1 // wire to GND
#define IGNITER_ON LOW
#define IGNITER_OFF HIGH
Bounce igniterButton = Bounce();

// also used
// D4, GPIO2, BUILTIN_LED

// PWM granuality
void setPwmRange(int val) {
  val = constrain(val, 0, 65535);
  Serial << "Flame.  Setting PWM range to: 0-" << val << endl;
  analogWriteRange(val);
}
// PWM frequency
void setPwmFreq(int val) {
  val = constrain(val, 0, 10000);
  Serial << "Flame.  Setting PWM frequency to: " << val << endl;
  analogWriteFreq(val);
}
// target for ramps
rampInt valveRamp[3];
// ramp time
unsigned long rampTime;
void setRampTime(int val) {
  rampTime = constrain((unsigned long)val, 0UL, 65535UL);
  Serial << "Flame.  Setting ramp time to: " << rampTime << endl;
}
// interpolation
enum ramp_mode rampMode;
void setRampMode(int val) {
  rampMode = constrain((ramp_mode)val, NONE, BOUNCE_INOUT);
  Serial << "Flame.  Setting ramp mode to: " << rampMode << endl;
}
// loop mode
enum loop_mode rampLoop;
void setRampLoop(int val) {
  rampLoop = constrain((loop_mode)val, ONCEFORWARD, BACKANDFORTH);
  Serial << "Flame.  Setting ramp loop to: " << rampLoop << endl;
}

void rampValves() {
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
  for( byte i=0; i<8; i++ ) Comms.sub( Comms.actBeaconFlame[i] );

  // configure proportional valves
  setPwmFreq(60); //Hz
  setPwmRange(1023); // 0..1023
  setRampTime(5000UL); // ms
  setRampMode(LINEAR); // see Ramp.h for options
  setRampLoop(ONCEFORWARD); // ibid
  
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
  if ( topic.indexOf("/A/") != -1 ) valveRamp[0].go(val, rampTime, rampMode, rampLoop);
  if ( topic.indexOf("/B/") != -1 ) valveRamp[1].go(val, rampTime, rampMode, rampLoop);
  if ( topic.indexOf("/C/") != -1 ) valveRamp[2].go(val, rampTime, rampMode, rampLoop);

  if ( topic.indexOf("pwmrange") != -1 ) setPwmRange(val); 
  if ( topic.indexOf("pwmfreq") != -1 ) setPwmFreq(val); 
  
  if ( topic.indexOf("ramptime") != -1 ) setRampTime((unsigned long)val);
  if ( topic.indexOf("rampmode") != -1 ) setRampMode(val);
  if ( topic.indexOf("ramploop") != -1 ) setRampLoop(val);
}
