// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Ramp.h>

// Assumptions:

// 1. proportional control valve power is 24VDC, and the supply is controlled by a switch.

// 2. each tower has a proportional control valve:
//    SD8202G004V: 35 psi max, 1/4" NPT, 0.50 Cv
//    We control these by a MOSFET and PWM.
#define PROP_A_PIN D5

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
}

void setup() {
  // set them off, then enable pin.
  analogWrite(PROP_A_PIN, 0); pinMode(PROP_A_PIN, OUTPUT);
  valveRamp[0].go(0, 0, NONE, ONCEFORWARD);
 
  // wait a tic
  delay(250);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // configure proportional valves
  setPwmFreq(300); //Hz
  setPwmRange(1023); // 0..1023
  setRampTime(5000UL); // ms
  setRampMode(LINEAR); // see Ramp.h for options
  setRampLoop(FORTHANDBACK); // ibid
  
  Serial << F("Startup complete.") << endl;
}

void loop() {
  // flame handling
  rampValves();

  if( Serial.available()>0 ) {
    delay(5);
    char c = Serial.read();
    int val = Serial.parseInt();
    switch(c) {
      case 'l': setRampLoop(val); break;
      case 'm': setRampMode(val); break;
      case 't': setRampTime(val); break;
    }
    valveRamp[0].go(0);
    valveRamp[0].go(1023, rampTime, rampMode, rampLoop);  
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {
  // amount?
  int val = constrain(message.toInt(), 0, 65535);

  // do it
  if ( topic.indexOf("/A/") != -1 ) valveRamp[0].go(val, rampTime, rampMode, rampLoop);

  if ( topic.indexOf("pwmrange") != -1 ) setPwmRange(val); 
  if ( topic.indexOf("pwmfreq") != -1 ) setPwmFreq(val); 
  
  if ( topic.indexOf("ramptime") != -1 ) setRampTime((unsigned long)val);
  if ( topic.indexOf("rampmode") != -1 ) setRampMode(val);
  if ( topic.indexOf("ramploop") != -1 ) setRampLoop(val);
}
