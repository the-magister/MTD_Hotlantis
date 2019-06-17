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

// wire it up
#define FOG_PIN D1
#define ON HIGH
#define OFF LOW

// D4, GPIO2, BUILTIN_LED, TDX1
// connected to "DI" on MAX3485 
DMXESPSerial dmx;
#define DMX_CHAN_A 1
#define DMX_CHAN_B 2
#define DMX_CHAN_C 3
#define DMX_CHAN_D 4

void setup() {
  // set them off, then enable pin.
  digitalWrite(FOG_PIN, OFF); 
  pinMode(FOG_PIN, OUTPUT);

  // DMX
  dmx.init();               // initialization for first 32 addresses by default
  //dmx.init(512)           // initialization for complete bus
  delay(200);               // wait a while (not necessary)

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap new microcontrollers, if needed.
  if ( true ) {
    Comms.saveStuff("myName", "FogBubbleStuff");
  }

  // configure comms
  // Note, I'm turning off heartbeat.  That pin is used for DMX control.
  Comms.begin(Comms.loadStuff("myName"), processMessages, 99); 
  // subs
  Comms.sub(Comms.actMTDFogBubbleEtc[0]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[1]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[2]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[3]); 
  Comms.sub(Comms.actMTDFogBubbleEtc[4]); 

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // update the DMX bus.  Not strictly neccessary, but get us a heartbeat
  // since the pin for DMX send is wired to the internal LED.
  static Metro updateDMX(5000UL);
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
  }
}
