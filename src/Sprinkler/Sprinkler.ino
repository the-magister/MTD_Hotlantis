// IDE Settings:
// Tools->Board : "WeMos D1 R2 & mini"
// Tools->Flash Size : "4M (3M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

// wire it up
// devices with the light shield have access to D5-D8
#define PUMP_A D0
#define PUMP_B D5
#define SPRINKLER_A D6
#define SPRINKLER_B D7
#define SPRINKLER_C D8
#define ON HIGH
#define OFF LOW
// also used
// D4, GPIO2, BUILTIN_LED

void setup() {
  // set them off, then enable pin.
  digitalWrite(PUMP_A, OFF); pinMode(PUMP_A, OUTPUT);
  digitalWrite(PUMP_B, OFF); pinMode(PUMP_B, OUTPUT);
  digitalWrite(SPRINKLER_A, OFF); pinMode(SPRINKLER_A, OUTPUT);
  digitalWrite(SPRINKLER_B, OFF); pinMode(SPRINKLER_B, OUTPUT);
  digitalWrite(SPRINKLER_C, OFF); pinMode(SPRINKLER_C, OUTPUT);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // configure comms
  Comms.begin("Sprinkler", processMessages);

  // subscribe
  // noting that you CANNOT do the obvious thing and pass the String array members.  pointers.
  Comms.addSubscription("nyc/interaction/#");
  
  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();
}

// pump manipulation.  KISS.
void adjustPumps(String topic, String message) {
  // tracking
  static boolean activeSprinklers[] = {false, false, false};
  static boolean activePumps[] = {false, false};

  // if we're not idle, then we need one or more pumps
  boolean needPump = ! message.equals(Comms.messageInteraction[0]); // if not idle

  // which sprinkler is topical?
  if( topic.endsWith("A") ) activeSprinklers[0]=needPump;
  if( topic.endsWith("B") ) activeSprinklers[1]=needPump;
  if( topic.endsWith("C") ) activeSprinklers[2]=needPump;
  
//  Serial << "need pumps? " << pumpsNeeded[0] << " " << pumpsNeeded[1] << " " << pumpsNeeded[2] << endl;

  // how many sprinklers and pumps are active?
  byte sumActiveSprinklers = activeSprinklers[0] + activeSprinklers[1] + activeSprinklers[2];
  byte sumActivePumps = activePumps[0] + activePumps[1];
 
}


// processes messages that arrive
void processMessages(String topic, String message) {
  // bail out if we're not processing our interaction
  if( ! topic.startsWith("nyc/interaction") ) return;

  // do we still need the pumps?
  adjustPumps(topic, message);
}

