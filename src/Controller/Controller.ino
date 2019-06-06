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
  static Metro soundTimer(15UL * 1000UL);
  if ( soundTimer.check() && !playedIntro) {
      char msg[2];
      itoa(300,msg,10);
      actState.soundAction = "PlaySolo";
      actState.soundTrack = msg;
	  // MGD: I think this is easier:
	  // actState.soundTrack = String(300); // base 10 is the default

      Serial << "***Playing intro sound" << endl;
      playedIntro = true;

      String smsg = String();
      smsg.concat(Comms.actSound[0]);
      smsg.concat(actState.soundAction);
	  // MGD: I think the "+" operator is ok, too:
	  // String smsg = Comms.actSound[0] + actState.soundAction;
	  
	  
	  // Still, I think you want to enumerate actSound:
	  //
	  // enum SoundAction = { PlaySolo, Play, Quiet, ...};
	  // put that in MTD_Hotlantis.h? 
	  //
	  // In actState, define soundAct as that enumerated type.
	  // 
	  // then:
	  // Comms.pub(Comms.actSound[actState.soundAct], actState.soundTrack);
	  
      Serial << "Sending sound: " << smsg << endl;
      Comms.pub(smsg, actState.soundTrack);
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

  // Any time effects
  if ( sensorData.buttonCannon[0] || sensorData.buttonCannon[1] ) {
      Serial << "Playing Cannon Sound" << endl;
      char msg[2];
      itoa(951,msg,10);
      actState.soundAction = "PlaySoloLoop";
      actState.soundTrack = msg;
  } else if (!sensorData.buttonCannon[0] && !sensorData.buttonCannon[1]) {
      char msg[2];
      itoa(951,msg,10);
      actState.soundAction = "Stop";
      actState.soundTrack = msg;
  }
  
  // note we've done with updates
  sensorData.haveChanged = false;
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
    sensorData.buttonMTD[index] = (boolean)message.toInt();
  } else if ( topic.indexOf("motion") != -1 && index != 99  ) {
    sensorData.motionMTD[index] = (boolean)message.toInt();
  } else if ( topic.indexOf("cannon") != -1 && index != 99  ) {
//    sensorData.buttonCannon[index] = (boolean)message.toInt();
    Serial << "Handle cannon message.  " << message << endl;

    // Handle as strings first and binary second
    if (message[0] == '0') {
      Serial << "Cannon released" << endl;
      sensorData.buttonCannon[index] = false;
    } else if (message[0] == '1') {
      Serial << "Cannon fired.  index: " << index << endl;
      sensorData.buttonCannon[index] = true;      
    } else {
      Serial << "Got a weird cannon value: " << message[0] << endl;
    }
  } else {
    Serial << "I'm not sure why, but here we are. index=" << index << endl;
  }
  // flag a change
  sensorData.haveChanged = true;
  // show the new state
  printSensorData();
}
