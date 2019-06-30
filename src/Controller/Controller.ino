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
#include <FiniteStateMachine.h>
#include <CircularBuffer.h>

void idleEnter(), idle(), idle(); State Idle = State(idleEnter, idle, NULL);
void gameplayEnter(), gameplay(); State Gameplay = State(gameplayEnter, gameplay, NULL);
void customAnimationEnter(), customAnimation(); State CustomAnimation = State(customAnimationEnter, customAnimation, NULL);
void winEnter(), win(); State Win = State(winEnter, win, NULL);

#define A 0
#define B 1
#define C 2

// Debuging
#define SILENCE false
#define FORCE_DAYTIME false
#define FORCE_NIGHTIME true

// Gameplay
#define INTERACTIONS_TO_WIN 30

FSM stateMachine = FSM(Idle); // initialize state machine
static Metro inactiveTimer(15UL * 1000UL);
static Metro winTimer(30UL * 1000UL);
static Metro customAnimationTimer(30UL * 1000UL);
int numInteractions;  // The number of player interactions since gameplay start
CircularBuffer<byte,7> buttonQueue;
static int infinite = 60*60*1000UL;
static int buttonFlash = 1UL * 500UL;
static Metro buttonMTDTimer[3] = {Metro(infinite), Metro(infinite), Metro(infinite)}; 
static int winLockout = 2 * 60UL * 1000UL;
static Metro winLockoutTimer(infinite);

//Cheat codes
struct CheatCode_t {
  long hash;
  char * code;
  String name;
  int track;
};

#define NUM_CHEATS 32
CheatCode_t cheats[NUM_CHEATS];
int activeCheats;

// Sounds
#define LONELY_START 100
#define OHAI_START 200
#define GOODNUF_START 400
#define GOODJOB_START 500
#define WINNING_START 600
#define FANFARE_START 300
#define BEAT_START 700
#define POS_CHANGE_START 800
#define NEG_CHANGE_START 900
#define GUNFIRE_START 950

#define LONELY_NUM 10
#define OHAI_NUM 14
#define FANFARE_NUM 69
#define GOODNUF_NUM 7  // 1
#define GOODJOB_NUM 4  // 2
#define WINNING_NUM 3  // 1
#define BEAT_NUM 30
#define POS_CHANGE_NUM 18
#define NEG_CHANGE_NUM 22
#define GUNFIRE_NUM 11
#define PALETTE_NUM 0

// Buttons for faker
// wire it up
const boolean onReading = false;
// devices with the light shield block access to D5-D8
#define PIN_BUTTON_A D1 // wire D3/GPIO0 through a N.O. switch to GND
#define PIN_BUTTON_B D2 // wire D3/GPIO0 through a N.O. switch to GND
#define PIN_BUTTON_C D3 // wire D3/GPIO0 through a N.O. switch to GND
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();

/*
   Collect all the "sense" topics, decide, then publish "act" topics.

   Controller.  That's it.
*/

// Collect all the "sense" topics.
struct SensorData_t {
  // flag a change
  boolean haveChanged;

  // clock data
  byte timeDayOfWeek, timeHour;

  // MTD inputs
  boolean motionMTD[3]; // A,B,C
  boolean buttonMTD[3]; // A,B,C

  // cannon inputs
  boolean buttonCannon[2] = {false,false}; // L,R
};
SensorData_t sensorData;

// Publish the "act" topics.
struct ActuatorState_t {
  // fog outputs
  boolean fogMTD; // on,off
  boolean bubbleMTD;

  // water outputs
  boolean primePump[2]; // on,off
  boolean boostPump[2]; // on,off
  boolean cannonSpray; // on,off
  boolean beaconSpray[3]; // on,off

  // fire outputs
  boolean beaconIgniter[3]; // on,off
  int beaconFlame[3]; // 0=off, 255=maximal
  // MGD: incorrect.  Max is PWMRANGE.  See Flame.ino.  You can also set the bit depth.

  // light outputs
  // these are far, far more complicated than "on" or "off".
  // you must get into the associated uC code and "agree" on what messages generate what actions
  // in theory, you can send a serialized LED pattern.
  String lightMTD[3]; // A,B,C
  String lightBeacon[3]; // A,B,C

  // sound outputs
  // likewise, very complicated.
  String soundAction;
  String soundTrack;
  
};
ActuatorState_t actState;

// also used
// D4, GPIO2, BUILTIN_LED

String someVeryImportantThing;

void setup() {
  // for random numbers
  randomSeed(analogRead(0));

  // Call me paranoid, but I like to ensure programmability.
  delay(150);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( true ) {
    Comms.saveStuff("whatsup", "Some information you need, later.");
    Comms.saveStuff("morestuff", "andagain.");
  }
  someVeryImportantThing = Comms.loadStuff("whatsup");

  // configure comms
  Comms.begin("gwf/controller", processSensors);
  Comms.sub("gwf/s/#"); // every sense topic

  initCheatCodes();

  // Setup fake buttons for testing
  buttonA.attach(PIN_BUTTON_A, INPUT_PULLUP);
  buttonA.interval(5); // interval in ms
  buttonB.attach(PIN_BUTTON_B, INPUT_PULLUP);
  buttonB.interval(5); // interval in ms
  buttonC.attach(PIN_BUTTON_C, INPUT_PULLUP);
  buttonC.interval(5); // interval in ms
  
  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // the complex part of Controller's job
  if ( sensorData.haveChanged ) {
    // determine what actions we should take, given the sensor readings
    mapSensorsToActions();
    // make those actions happen
    publishActions();
  }

  // I don't know.  What else do you want to do, periodically?
  static Metro someTimer(10UL * 1000UL);
  if ( someTimer.check() ) {
    printSensorData();
  }

  static bool playedIntro = false;
  static Metro soundTimer(3UL * 1000UL);
  if ( soundTimer.check() && !playedIntro) {
      if (Comms.getStatus() != FULL_CONNECTION) return;
      
      Serial << "WIFI Connected" << endl;
      
      playTrack("PlaySolo", 100);
      playedIntro = true;
      	  
  }

  if ( inactiveTimer.check() ) {
    if (stateMachine.isInState(Gameplay)) {

      stateMachine.transitionTo(Idle);  
    }
  }
  if ( winTimer.check() ) {
    if (stateMachine.isInState(Win)) {

      stateMachine.transitionTo(Idle);  
    }
  }
  if ( customAnimationTimer.check() ) {
    if (stateMachine.isInState(CustomAnimation)) {

      stateMachine.transitionTo(Idle);  
    }
  }

  for(int i=0; i < 3; i++) {
    if (buttonMTDTimer[i].check()) {
      actState.lightBeacon[i] = "black";
      sensorData.haveChanged = true;
    }    
  }

  buttonFaker();
  
  stateMachine.update();
}

void buttonFaker() {
    // update buttons
  if ( buttonA.update() ) {
    // publish new reading.
    Comms.pub(Comms.senseMTDButton[0], Comms.messageBinary[buttonA.read() == onReading]);
  }
  if ( buttonB.update() ) {
    // publish new reading.
    Comms.pub(Comms.senseMTDButton[1], Comms.messageBinary[buttonB.read() == onReading]);
  }
  if ( buttonC.update() ) {
    // publish new reading.
    Comms.pub(Comms.senseMTDButton[2], Comms.messageBinary[buttonC.read() == onReading]);
  }
}
void setColorPalette(byte timeDayOfWeek) {
  String pal;
  switch ( timeDayOfWeek ) {
    case 0: pal = "CloudColors_p"; break;
    case 1: pal = "LavaColors_p"; break;
    case 2: pal = "OceanColors_p"; break;
    case 3: pal = "ForestColors_p"; break;
    case 4: pal = "RainbowColors_p"; break;
    case 5: pal = "RainbowStripeColors_p"; break;
    case 6: pal = "PartyColors_p"; break;
  }
  for ( byte i = 0; i < 3; i++ ) actState.lightMTD[i] = actState.lightBeacon[i] = pal;
}

// Initialize the cheat codes
void initCheatCodes() {
  int idx = 0;
  setCheat(idx++,"aaabbbc",0); // Test
  //setCheat(idx++,"aaabbcb",0); // Alan
  //setCheat(idx++,"aabaaca",1);  // Mike

  activeCheats = idx;
}

void setCheat(int idx, char * code, int track) {
  cheats[idx].code = code;
  cheats[idx].hash = calcCheatHash(code);
  cheats[idx].track = track;
}

long calcCheatHash(char * cheat) {
  long hash = (cheat[0]-'a') + (cheat[1] - 'a') * 3 + (cheat[2] - 'a') * 9 + 
    (cheat[3]-'a') * 27 + (cheat[4] - 'a') * 81 + (cheat[5] - 'a') * 243 + (cheat[6] - 'a') * 729;

   Serial << "Cheat: " << cheat << " hash: " << hash << endl; 
   return hash;
}

int checkCheats() {
  if (buttonQueue.size() < 7) return -1;
  
  long hash = 0;
  for(int i=0; i < 7; i++) {
    hash += buttonQueue[i] * pow(3,i);  
  }

  Serial << "Current cheat hash: " << hash << endl;
  for(int i=0; i < activeCheats; i++) {
    if (cheats[i].hash == hash) {
      Serial << "*** Got cheat code: " << cheats[i].code << endl;
      return cheats[i].track;  
    }
  }

  return -1;
}

void mapSensorsToActions() {
  // Clock
  // Maybe rotate colorpalette based on day of week?
  static byte timeDayOfWeek, timeHour;
  if ( sensorData.timeDayOfWeek != timeDayOfWeek ) {
    Serial << "Change of day" << endl;
    
    timeDayOfWeek = sensorData.timeDayOfWeek;
    setColorPalette(timeDayOfWeek);
    return; // bail out, so the message is sent.  Important, as we might clobber the light message downcode.
  }

  // Daytime: water, sound
  // Nightime: flame, light, fog, sound
  if( FORCE_DAYTIME || (!FORCE_NIGHTIME && sensorData.timeHour >= 5 && sensorData.timeHour < (8+12)) ) {
    // Daytime

    // Water
    byte countSprayers = 
      ( sensorData.buttonCannon[0] || sensorData.buttonCannon[1] ) +
      ( sensorData.buttonMTD[0] ) +
      ( sensorData.buttonMTD[1] ) +
      ( sensorData.buttonMTD[2] );

    countSprayers = 4; // Hardcode

    // daily rotation of pump that is most heavily loaded
    byte mainPump = constrain(sensorData.timeDayOfWeek % 2, 0, 1); // never index into an array without constrain()
    byte subPump = 1 - mainPump;
    
    // adjust pump count, if needed.
    actState.primePump[mainPump] = countSprayers > 0;
    actState.boostPump[mainPump] = countSprayers > 1;
    actState.primePump[subPump] = countSprayers > 2;
    actState.boostPump[subPump] = countSprayers > 3;
    
    // adjust the sprayers
    actState.cannonSpray = sensorData.buttonCannon[0] || sensorData.buttonCannon[1];
    actState.beaconSpray[0] = sensorData.buttonMTD[0];
    actState.beaconSpray[1] = sensorData.buttonMTD[1];
    actState.beaconSpray[2] = sensorData.buttonMTD[2];

    // Light.  Turn it all off to save power
    actState.lightBeacon[A] = "off";
    actState.lightBeacon[B] = "off";
    actState.lightBeacon[C] = "off";
    actState.lightMTD[A] = "off";
    actState.lightMTD[B] = "off";
    actState.lightMTD[C] = "off";

    // Sound
//    if ( sensorData.buttonCannon[0] || sensorData.buttonCannon[1] ) actState.sound = "cannonOn";
//    else actState.sound = "cannonOff";

  } else {
    // Nighttime

    // Light

    actState.lightMTD[A] = "rainbow";
    actState.lightMTD[B] = "rainbow";
    actState.lightMTD[C] = "rainbow";
    
    for(int i=0; i < 3; i++) {
      if (sensorData.buttonMTD[i]) {
        // Flash the light to 
        actState.lightBeacon[i] = "centerWhite";
        buttonMTDTimer[i].interval(buttonFlash);
        buttonMTDTimer[i].reset();
      }
    }
    
    // Flame
    // try not to blow everything up.
    // did you run the igniter before fire?

    // Fog 
    
    // Sound
  }

  if (stateMachine.isInState(Idle) && 
     (sensorData.buttonMTD[0] || sensorData.buttonMTD[1] || sensorData.buttonMTD[2] ||
      sensorData.motionMTD[0] || sensorData.motionMTD[1] || sensorData.motionMTD[2] ||
      sensorData.buttonCannon[0] || sensorData.buttonCannon[1])) {
    stateMachine.transitionTo(Gameplay);  
  }

  static int cannonTrack = -1;
  
  // Any time effects
  if ( cannonTrack == -1 && (sensorData.buttonCannon[0] || sensorData.buttonCannon[1] )) {
      Serial << "Playing Cannon Sound" << endl;

      cannonTrack = chooseTrack(GUNFIRE_START,GUNFIRE_NUM);
      playTrack("PlayLoop",cannonTrack);
  } 
  
  if (cannonTrack >= 0 && (!sensorData.buttonCannon[0] && !sensorData.buttonCannon[1])) {
    /*
      char msg[2];
      itoa(cannonTrack,msg,10);
      actState.soundAction = "Stop";
      actState.soundTrack = msg;
      */
      playTrack("Stop",cannonTrack);
      cannonTrack = -1;
  }
  
  // note we've done with updates
  sensorData.haveChanged = false;
}

void playTrack(String action, int track) {
  
  if (SILENCE) {
    Serial << "Silenced.  act: " << action << " track: " << track << endl;
    return;
  } else {
    Serial << "Playing.  act: " << action << " track: " << track << endl;  
  }

  char msg[2];
  itoa(track,msg,10);
  actState.soundAction = action;
  actState.soundTrack = msg;
  sensorData.haveChanged = true;
}

void publishActions() {
  // save, so we're only publishing the delta
  static ActuatorState_t lastState;

  // fog/bubble outputs
  if ( lastState.fogMTD != actState.fogMTD ) publishBinary(Comms.actMTDFogBubbleEtc[0], actState.fogMTD);
  if ( lastState.bubbleMTD != actState.bubbleMTD ) publishBinary(Comms.actMTDFogBubbleEtc[1], actState.bubbleMTD);

  // sound outputs
  if ( lastState.soundAction != actState.soundAction || lastState.soundTrack != actState.soundTrack ) {

    String msg = String();
    msg.concat(Comms.actSound[0]);
    msg.concat(actState.soundAction);
    Serial << "Sending sound: " << msg << endl;
    Comms.pub(msg, actState.soundTrack);
  }

  // water outputs
  //if ( lastState.primePump[0] != actState.primePump[0] ) publishBinary(Comms.actPump[0], actState.primePump[0]);
  //if ( lastState.primePump[1] != actState.primePump[1] ) publishBinary(Comms.actPump[1], actState.primePump[1]);
  //if ( lastState.boostPump[0] != actState.boostPump[0] ) publishBinary(Comms.actPump[2], actState.boostPump[0]);
  //if ( lastState.boostPump[1] != actState.boostPump[1] ) publishBinary(Comms.actPump[3], actState.boostPump[1]);
  //if ( lastState.cannonSpray != actState.cannonSpray )   publishBinary(Comms.actPump[4], actState.cannonSpray);

  for ( byte i = 0; i < 3; i++ ) {
    // nav beacon sprayers
    /*
    if ( lastState.beaconSpray[i] != actState.beaconSpray[i] )
      publishBinary(Comms.actBeaconSpray[i], actState.beaconSpray[i]);
*/

    // nav beacon igniters
    if ( lastState.beaconIgniter[i] != actState.beaconIgniter[i] )
      publishBinary(Comms.actBeaconIgniter[i], actState.beaconIgniter[i]);

    // nav beacon flame
    if ( lastState.beaconFlame[i] != actState.beaconFlame[i] )
      Comms.pub(Comms.actBeaconFlame[i], String( actState.beaconFlame[i], DEC ));

    // nav beacon lights
    if ( lastState.lightBeacon[i] != actState.lightBeacon[i] )
      Comms.pub(Comms.actBeaconLight[i], actState.lightBeacon[i]);

    // MTD lights
    if ( lastState.lightMTD[i] != actState.lightMTD[i] )
      Comms.pub(Comms.actMTDLight[i], actState.lightMTD[i]);
  }

  // save
  lastState = actState;
}

// just tidies the code a little bit
void publishBinary(String topic, boolean state) {
  Comms.pub(topic, state ? Comms.messageBinary[1] : Comms.messageBinary[0]);
}

void printSensorData() {
  Serial << "Controller. Sensors:";
  Serial << " update=" << sensorData.haveChanged;
  Serial << " DoW=" << sensorData.timeDayOfWeek;
  Serial << " Hour=" << sensorData.timeHour;
  Serial << " Cannon=" << sensorData.buttonCannon[0] << sensorData.buttonCannon[1];
  Serial << " Button=" << sensorData.buttonMTD[0] << sensorData.buttonMTD[1] << sensorData.buttonMTD[2];
  Serial << " Motion=" << sensorData.motionMTD[0] << sensorData.motionMTD[1] << sensorData.motionMTD[2];
  Serial << endl;
}

void idleEnter() {
  Serial << "Entered Idle" << endl;

  // Reset state
  actState.primePump[0] = false;
  actState.primePump[1] = false;
  actState.boostPump[0] = false;
  actState.boostPump[1] = false;
  actState.cannonSpray = false;
  actState.beaconSpray[0] = false;
  actState.beaconSpray[1] = false;
  actState.beaconSpray[2] = false;
  actState.beaconIgniter[A] = false;
  actState.beaconFlame[A] = 0;
  actState.beaconIgniter[B] = false;
  actState.beaconFlame[B] = 0;
  actState.beaconIgniter[C] = false;
  actState.beaconFlame[C] = 0;
  actState.soundAction = "";
  actState.soundTrack = "";
  sensorData.haveChanged = true;
  
  numInteractions = 0;
  buttonQueue.clear();
}

void idle() {
  
}

void gameplayEnter() {
  Serial << "Entered Gameplay" << endl;

  winLockoutTimer.interval(winLockout);
  winLockoutTimer.reset();

  // Play OHAI
  playTrack("PlaySolo",chooseTrack(OHAI_START,OHAI_NUM));

  sensorData.haveChanged = true;

}

void gameplay() {
  
}

static Metro flameTimer[3] = {Metro(infinite), Metro(infinite), Metro(infinite)}; 
static Metro ignitorTimer[3] = {Metro(infinite), Metro(infinite), Metro(infinite)}; 


void customAnimationEnter() {
  Serial << " Custom Animation Started" << endl;
  customAnimationTimer.reset();

  // Shoot some flame.  TODO: Account for time of day here or elsewhere?
  actState.beaconIgniter[A] = true;
  actState.beaconFlame[A] = 512;
  ignitorTimer[A].interval(3UL * 1000UL);
  ignitorTimer[A].reset();
  flameTimer[A].interval(10UL * 1000UL);
  flameTimer[A].reset();
  sensorData.haveChanged = true;
}


// Make sure we have flame ignition
boolean flameIgnited[] = {false,false,false};  // Do we think the flame is ignited

void insureIgnition(int beacon) {
  if (flameIgnited[beacon]) {
    return;    
  } else {
    actState.beaconIgniter[beacon] = true;
    actState.beaconFlame[beacon] = 375;
    ignitorTimer[beacon].interval(3UL * 1000UL);
    ignitorTimer[beacon].reset();
    flameIgnited[beacon] = true;
    sensorData.haveChanged = true;    
  }
}

void customAnimation() {
  // Turn off ignitor and flame
  for(int i=0; i < 3; i++) {
    if (flameTimer[i].check()) {
      actState.beaconFlame[i] = 0;
      sensorData.haveChanged = true;
    }
    if (ignitorTimer[i].check()) {
      actState.beaconIgniter[i] = false;
      sensorData.haveChanged = true;
    }
  }
}

void winEnter() {
    Serial << "Entered Win" << endl;
  
  // Play WIN
  playTrack("PlaySolo",chooseTrack(FANFARE_START,FANFARE_NUM));

  sensorData.haveChanged = true;
  winTimer.reset();
}

void win() {
}

// randomly choose a track from the available 
int chooseTrack(int strack, int len) {
  return (int) random(strack,strack+len); 
}

void registerInteraction() {
    inactiveTimer.reset();    
    numInteractions++;

    Serial << "NumInteractions: " << numInteractions << endl;
    if (stateMachine.isInState(Gameplay) && numInteractions >= INTERACTIONS_TO_WIN && winLockoutTimer.check()) {
      stateMachine.transitionTo(Win);  
    }
}

// processes messages that arrive
void processSensors(String topic, String message) {
  // is there an index A,B,C?
  byte index = 99;
  if ( topic.indexOf("/A/") != -1 ) index = 0;
  if ( topic.indexOf("/B/") != -1 ) index = 1;
  if ( topic.indexOf("/C/") != -1 ) index = 2;
  if ( topic.endsWith("/L") ) index = 0;
  if ( topic.endsWith("/R") ) index = 1;

  // check the topics, store the message.
  if ( topic.indexOf("clock") != -1 ) {
    if ( topic.indexOf("hour") != -1) {
      sensorData.timeHour = (byte)message.toInt();
    }
    if ( topic.indexOf("dayofweek") != -1) {
      sensorData.timeDayOfWeek = (byte)message.toInt();
    }
  } else if ( topic.indexOf("button") != -1 && index != 99 ) {
    sensorData.buttonMTD[index] = Comms.convBinary(message[0]);
    if (sensorData.buttonMTD[index]) {
      registerInteraction();
      buttonQueue.push(index);
      Serial << "buttonQueue size: " << buttonQueue.size() << endl;
      int ctrack = checkCheats();
      if (ctrack > -1) {
       if (stateMachine.isInState(Gameplay)) {
          playTrack("PlaySolo",FANFARE_START + ctrack);

          sensorData.haveChanged = true;

          stateMachine.transitionTo(CustomAnimation);  
        }     
      }
    }
    Serial << "Got a button" << endl;
  } else if ( topic.indexOf("motion") != -1 && index != 99  ) {
    sensorData.motionMTD[index] = (boolean)message.toInt();
    if (sensorData.motionMTD[index]) registerInteraction();
  } else if ( topic.indexOf("cannon") != -1 && index != 99  ) {
    Serial << "Handle cannon message.  " << message << endl;
    inactiveTimer.reset();  // Don't register interactions here, we want tower interactions for winning
    sensorData.buttonCannon[index] = Comms.convBinary(message[0]);
  } else {
    Serial << "I'm not sure why, but here we are. index=" << index << endl;
  }
  // flag a change
  sensorData.haveChanged = true;
  // show the new state
  printSensorData();
}
