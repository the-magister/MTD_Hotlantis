// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"
// Tools->Erase Flash : "Sketch Only"

#include <Streaming.h>
#include <Metro.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <ESPDMX.h>
#include <MTD_Hotlantis.h>
#include <FastLED.h>

// wire it up
#define FOG_PIN D1
#define ON HIGH
#define OFF LOW

// D4, GPIO2, BUILTIN_LED, TDX1
// connected to "DI" on MAX3485 
DMXESPSerial dmx;
#define DMX_CHAN_A 1 // bubbler
#define DMX_CHAN_B 2 // unassigned?
#define DMX_CHAN_C 3 // unassigned?
#define DMX_CHAN_D 4 // unassigned?

#define DMX_SPOT_A_MASTERB 8
#define DMX_SPOT_A_RED DMX_SPOT_A_MASTERB+1
#define DMX_SPOT_A_GRN DMX_SPOT_A_MASTERB+2
#define DMX_SPOT_A_BLU DMX_SPOT_A_MASTERB+3
#define DMX_SPOT_A_WHT DMX_SPOT_A_MASTERB+4

#define DMX_SPOT_B_MASTERB 13
#define DMX_SPOT_B_RED DMX_SPOT_B_MASTERB+1
#define DMX_SPOT_B_GRN DMX_SPOT_B_MASTERB+2
#define DMX_SPOT_B_BLU DMX_SPOT_B_MASTERB+3
#define DMX_SPOT_B_WHT DMX_SPOT_B_MASTERB+4

#define DMX_SPOT_C_MASTERB 18
#define DMX_SPOT_C_RED DMX_SPOT_C_MASTERB+1
#define DMX_SPOT_C_GRN DMX_SPOT_C_MASTERB+2
#define DMX_SPOT_C_BLU DMX_SPOT_C_MASTERB+3
#define DMX_SPOT_C_WHT DMX_SPOT_C_MASTERB+4

#define DMX_SPOT_D_MASTERB 23
#define DMX_SPOT_D_RED DMX_SPOT_D_MASTERB+1
#define DMX_SPOT_D_GRN DMX_SPOT_D_MASTERB+2
#define DMX_SPOT_D_BLU DMX_SPOT_D_MASTERB+3
#define DMX_SPOT_D_WHT DMX_SPOT_D_MASTERB+4

void setup() {
  // set them off, then enable pin.
  digitalWrite(FOG_PIN, OFF); 
  pinMode(FOG_PIN, OUTPUT);

  delay(1000);

  // DMX
  dmx.init();               // initialization for first 32 addresses by default
  //dmx.init(512)           // initialization for complete bus
  delay(200);               // wait a while (not necessary)

  // set up
  dmx.write(DMX_SPOT_A_MASTERB, 255);        // channal on
  dmx.write(DMX_SPOT_B_MASTERB, 255);        // channal on
  dmx.write(DMX_SPOT_C_MASTERB, 255);        // channal on
  dmx.write(DMX_SPOT_D_MASTERB, 255);        // channal on

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( false ) {
    Comms.saveStuff("myName", "FogBubbleStuff");
  }

  // configure comms
  // Note, I'm turning off heartbeat.  That pin is used for DMX control.
  Comms.begin(Comms.loadStuff("myName"), processMessages, 99); 

  // set QoS=1. Our WiFi connection from within the Fog machine is poor, and 
  // we'd like to get commands reliably.
  Comms.setMQTTQOS(1);
  
  // subs
  Comms.sub(Comms.actMTDFogBubbleEtc[0]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[1]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[2]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[3]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[4]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[5]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[6]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[7]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[8]); 

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // update the DMX bus.  
  static Metro updateDMX(10UL);
  if( updateDMX.check() && Comms.getStatus()==FULL_CONNECTION ) {
    dmx.update();
    updateDMX.reset();
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {

  // what action?  on/off?
  boolean setting = message.equals(Comms.messageBinary[1]) ? ON : OFF; // is on?

  if( topic.endsWith("fog") ) {
    digitalWrite(FOG_PIN, setting);  
  } else if( topic.endsWith("bubble") ) {
    dmx.write(DMX_CHAN_A, setting==ON ? 255 : 0);
    dmx.update();           
  } else if( topic.endsWith("chan2") ) {
    dmx.write(DMX_CHAN_B, setting==ON ? 255 : 0);
    dmx.update();          
  } else if( topic.endsWith("chan3") ) {
    dmx.write(DMX_CHAN_C, setting==ON ? 255 : 0);
    dmx.update();           
  } else if( topic.endsWith("chan4") ) {
    dmx.write(DMX_CHAN_D, setting==ON ? 255 : 0);
    dmx.update();           
  } else if( topic.endsWith("spotA") ) {
    byte hue = message.toInt();
    CRGB colorRGB = CHSV(hue, 255, 255);  
    dmx.write(DMX_SPOT_A_RED, colorRGB.red);
    dmx.write(DMX_SPOT_A_GRN, colorRGB.green);
    dmx.write(DMX_SPOT_A_BLU, colorRGB.blue);
    dmx.write(DMX_SPOT_A_WHT, 0);
    dmx.update();           
  } else if( topic.endsWith("spotB") ) {
    byte hue = message.toInt();
    CRGB colorRGB = CHSV(hue, 255, 255);  
    dmx.write(DMX_SPOT_B_RED, colorRGB.red);
    dmx.write(DMX_SPOT_B_GRN, colorRGB.green);
    dmx.write(DMX_SPOT_B_BLU, colorRGB.blue);
    dmx.write(DMX_SPOT_B_WHT, 0);
    dmx.update();           
  } else if( topic.endsWith("spotC") ) {
    byte hue = message.toInt();
    CRGB colorRGB = CHSV(hue, 255, 255);  
    dmx.write(DMX_SPOT_C_RED, colorRGB.red);
    dmx.write(DMX_SPOT_C_GRN, colorRGB.green);
    dmx.write(DMX_SPOT_C_BLU, colorRGB.blue);
    dmx.write(DMX_SPOT_C_WHT, 0);
    dmx.update();           
  } else if( topic.endsWith("spotD") ) {
    byte hue = message.toInt();
    CRGB colorRGB = CHSV(hue, 255, 255);  
    dmx.write(DMX_SPOT_D_RED, colorRGB.red);
    dmx.write(DMX_SPOT_D_GRN, colorRGB.green);
    dmx.write(DMX_SPOT_D_BLU, colorRGB.blue);
    dmx.write(DMX_SPOT_D_WHT, 0);
    dmx.update();           
  }
}
