// IDE Settings:
// Tools->Board : "LOLIN(WEMOS) D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

/* for a nice demo:
    find a level that's reliably 'just on"
    gwf/a/beacon/A/fire=350
    gwf/a/beacon/ramptime=5000
    gwf/a/beacon/rampmode=3
    gwf/a/beacon/ramploop=2
    gwf/a/beacon/A/fire=1023
*/

#include <Streaming.h>
#include <Metro.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <Bounce2.h>
#define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h>
#include <ESPDMX.h>
#include <ArduinoJson.h>
#include <MTD_Hotlantis.h>

/*
   types of messages:

   Publishes:
   digital input:
    digitalRead/XX/YY           // value YY of gpio XX

   Subscribes:
   digital output:
    gwf/genericModule/digitalWrite={gpio:2,val:0}
   analog output:
    gwf/genericModule/analogWrite={gpio:2,val:1000}
    gwf/genericModule/analogWrite={range:512}
    gwf/genericModule/analogWrite={freq:300}
   DMX output:
    gwf/genericModule/dmxWrite={chan:4,val:128}
    gwf/genericModule/dmxWrite={maxchan:64}

*/

// connect "D1" on MAX3485 to "D4/GPIO/BUILTIN_LED/TDX1" on ESP8266
DMXESPSerial dmx;
boolean updateDMX = false;

#define NO_LED 99

void setup() {
  // wait a tic
  delay(250);

  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // bootstrap, if needed.
  Comms.saveStuff("myName", "gwf/genericModule");
  String myName = Comms.loadStuff("myName");

  // configure comms
  Comms.begin(myName, processMessages, NO_LED);

  // set QoS=1.
  //  Comms.setMQTTQOS(1);

  // sub
  Comms.sub( myName + "/#" );

  Serial << F("Startup complete.") << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // updates?
  if ( updateDMX ) {
    EVERY_N_MILLISECONDS( 10 ) {
      dmx.update();
    }
  }
}

// processes messages that arrive
StaticJsonDocument<1024> doc;
void processMessages(String topic, String message) {

  DeserializationError error = deserializeJson(doc, message);

  // Test if parsing succeeded.
  if (error) {
    Serial << "deserializeJson() failed: " << endl;
    Serial << "topic: [" << topic << "]" << endl;
    Serial << "message: [" << message << "]" << endl;
    Serial << error.c_str() << endl;
    return;
  }

  // do it
  if ( topic.endsWith("digitalWrite") ) {
    if ( doc.containsKey("gpio") && doc.containsKey("val") ) {
      byte gpio = constrain(doc["gpio"], 0, 16);
      byte val = constrain(doc["val"], 0, 1);
      digitalWrite(gpio, val);
      pinMode(gpio, OUTPUT);
      Serial << "digitalWrite. GPIO=" << gpio << " value=" << val << endl;
    }
  } else if ( topic.endsWith("analogWrite") ) {
    // analog settings
    static int s_analogWriteRange = 1023;
    static int s_analogWriteFreq = 1000;

    if ( doc.containsKey("range") ) {
      s_analogWriteRange = doc["range"];
      analogWriteRange(s_analogWriteRange);
      Serial << "analogWrite. range=0-" << s_analogWriteRange << endl;
    }
    if ( doc.containsKey("freq") ) {
      s_analogWriteFreq = doc["freq"];
      analogWriteFreq(s_analogWriteFreq);
      Serial << "analogWrite. freq=" << s_analogWriteFreq << " Hz" << endl;
    }
    if ( doc.containsKey("gpio") && doc.containsKey("val") ) {
      byte gpio = constrain(doc["gpio"], 0, 16);
      int val = constrain(doc["val"], 0, s_analogWriteRange);
      analogWrite(gpio, val);
      pinMode(gpio, OUTPUT);
      Serial << "analogWrite. GPIO=" << gpio << " value=" << val << endl;
    }
  } else if ( topic.endsWith("dmxWrite") ) {

    // dmx settings
    static int s_MaxChan = 32;

    if ( doc.containsKey("maxchan") ) {
//      dmx.end();
      s_MaxChan = constrain(doc["maxchan"], 0, 512);
      updateDMX = false;
    }

    if ( ! updateDMX ) {
      dmx.init(s_MaxChan);
      updateDMX = true;
      Serial << "dmxWrite. initialized with maxchan=" << s_MaxChan << endl;
    }

    if ( doc.containsKey("chan") && doc.containsKey("val") ) {
      int chan = constrain(doc["chan"], 0, s_MaxChan);
      byte val = constrain(doc["val"], 0, 255);
      dmx.write(chan, val);
      Serial << "dmxWrite. chan=" << chan << " value=" << val << endl;
    }

  }

}
