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

// Debuging
#define SILENCE false

// Gameplay
#define INTERACTIONS_TOWIN 15

FSM stateMachine = FSM(Idle); // initialize state machine
static Metro inactiveTimer(15UL * 1000UL);
static Metro winTimer(30UL * 1000UL);
static Metro customAnimationTimer(30UL * 1000UL);
int numInteractions;  // The number of player interactions since gameplay start
CircularBuffer<byte,7> buttonQueue;

//Cheat codes
struct CheatCode_t {
  long hash;
  char * code;
  String name;
  int track;
};

#define NUM_CHEATS 32
CheatCode_t cheats[NUM_CHEATS];

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

  // water outputs
  boolean primePump[2]; // on,off
  boolean boostPump[2]; // on,off
  boolean cannonSpray; // on,off
  boolean beaconSpray[3]; // on,off

  // fire outputs
  boolean beaconIgniter[3]; // on,off
  byte beaconFlame[3]; // 0=off, 255=maximal
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


  stateMachine.update();
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
  setCheat(idx++,"bbbbbbb",0); // Test
  setCheat(idx++,"aaabbcb",0); // Alan
  setCheat(idx++,"aabaaca",1);  // Mike
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
  for(int i=0; i < NUM_CHEATS; i++) {
    if (cheats[i].hash == hash) {
      Serial << " Got cheat code: " << cheats[i].code;
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
  if( sensorData.timeHour >= 5 && sensorData.timeHour < (8+12) ) {
    // Daytime

    // Water
    byte countSprayers = 
      ( sensorData.buttonCannon[0] || sensorData.buttonCannon[1] ) +
      // are motion sensors and buttons the same?  
      ( sensorData.motionMTD[0] || sensorData.buttonMTD[0] ) +
      ( sensorData.motionMTD[1] || sensorData.buttonMTD[1] ) +
      ( sensorData.motionMTD[2] || sensorData.buttonMTD[2] );

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
    actState.beaconSpray[0] = sensorData.motionMTD[0] || sensorData.buttonMTD[0];
    actState.beaconSpray[1] = sensorData.motionMTD[1] || sensorData.buttonMTD[1];
    actState.beaconSpray[2] = sensorData.motionMTD[2] || sensorData.buttonMTD[2];

    // Sound
//    if ( sensorData.buttonCannon[0] || sensorData.buttonCannon[1] ) actState.sound = "cannonOn";
//    else actState.sound = "cannonOff";

  } else {
    // Nighttime

    // Light

    // Flame
    // try not to blow everything up.
    // did you run the igniter before fire?

    // Fog 
    
    // Sound
  }

  if (stateMachine.isInState(Idle) && 
     (sensorData.buttonMTD[0] || sensorData.buttonMTD[1] || sensorData.buttonMTD[2] ||
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

  // fog outputs
  if ( lastState.fogMTD != actState.fogMTD ) publishBinary(Comms.actMTDFog[0], actState.fogMTD);

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
  actState.soundAction = "";
  actState.soundTrack = "";
  numInteractions = 0;
  buttonQueue.clear();
}

void idle() {
  
}

void gameplayEnter() {
  Serial << "Entered Gameplay" << endl;
  
  // Play OHAI
  playTrack("PlaySolo",chooseTrack(OHAI_START,OHAI_NUM));

  sensorData.haveChanged = true;

}

void gameplay() {
  
}

void customAnimationEnter() {
  Serial << " Custom Animation Started" << endl;
  customAnimationTimer.reset();
}

void customAnimation() {
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
    if (stateMachine.isInState(Gameplay) && numInteractions >= INTERACTIONS_TOWIN) {
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
    if ( topic.indexOf("hour") ) sensorData.timeHour = (byte)message.toInt();
    if ( topic.indexOf("dayofweek") ) sensorData.timeDayOfWeek = (byte)message.toInt();
  } else if ( topic.indexOf("button") != -1 && index != 99 ) {
    sensorData.buttonMTD[index] = Comms.convBinary(message[0]);
    if (sensorData.buttonMTD[index]) {
      registerInteraction();
      buttonQueue.push(index);
      Serial << "buttonQueue size: " << buttonQueue.size() << endl;
      int ctrack = checkCheats();
      if (ctrack > -1) {
       if (stateMachine.isInState(Gameplay)) {
          playTrack("PlaySolo",chooseTrack(FANFARE_START,ctrack));

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
