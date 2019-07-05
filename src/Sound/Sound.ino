// IDE Settings:
// Tools->Board : "WeMos D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

#include <SoftwareSerial.h>
#include <Tsunami.h>

// Our Tsunami object
Tsunami tsunami;

// wire it up
// RX/TX for softserial comms
#define RX D1 // GPIO5.  wire to Tsunami TX0
#define TX D2 // GPIO4.  wire to Tsunami RX1
// talk to the Sound device device
SoftwareSerial mySerial(RX, TX, false, 1024);
// also used
// D4, GPIO2, BUILTIN_LED

void setup() {
  delay(250);
  
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // for remote output
  Serial << F("Configure Tsunami...");
  // Tsunami startup at 57600
  tsunami.start();
  delay(10);

  // Send a stop-all command and reset the sample-rate offset, in case we have
  //  reset while the Tsunami was already playing.
  tsunami.stopAllTracks();
  tsunami.samplerateOffset(0, 0);

  // Enable track reporting from the Tsunami
  tsunami.setReporting(true);

  // configure comms
  Comms.begin("gwf/a/sound", processMessages);
  Comms.sub("gwf/a/sound/#"); // all the sound topics.

  Serial << F("Startup complete.") << endl;
}

int waitForTrackEnd = -1;
Metro trackEndWait(1500UL);

void sendEndOfTrackMessage(int trackNum) {
  Comms.pub(Comms.senseSound[0], String(trackNum));
  waitForTrackEnd = -1;  
}

void loop() {
  // comms handling
  Comms.loop();
  
  // sound handling
  tsunami.update();

  if (waitForTrackEnd != -1 && trackEndWait.check() && !tsunami.isTrackPlaying(waitForTrackEnd)) {
    sendEndOfTrackMessage(waitForTrackEnd);
  }
}


// processes messages that arrive
void processMessages(String topic, String message) {
  
  // MGD: I'd recommend that these topic definitions get folder into MTD_Hotlantis.h
  // As an example, let's assume the following topics:
  // "Play"
  // "PlaySolo"
  // "PlayLoop"
  // "PlaySoloLoop" or "PlayLoopSolo" or whatever order you choose. *shrug*
  // "Quiet"
  //
  // message.toInt() is assumed to be a valid track number

  Serial << "Got message: " << topic << endl;
  
  if( topic.indexOf("Play") != -1 ) {
    // play a track
    
    // what track?
    int trackNum = message.toInt();
    
    // with modifiers
    boolean solo = topic.indexOf("Solo") != -1;
    tsunami.trackGain(trackNum, solo ? -5 : -10);

    boolean looped = topic.indexOf("Loop") != -1;
    tsunami.trackLoop(trackNum, looped);

    // fire it up
    if( solo ) {
      if (waitForTrackEnd != -1) {
        sendEndOfTrackMessage(waitForTrackEnd);  
      }
      // if you can, call Solo.  The Tsunami _will_ run out of polyphonic track slots, eventually.
      tsunami.stopAllTracks(); 
      tsunami.trackPlaySolo(trackNum, 0, true);
      waitForTrackEnd = trackNum;
      trackEndWait.reset();
    } else {
      tsunami.trackPlayPoly(trackNum, 0, true);   
    }
  } else if( topic.indexOf("Quiet") != -1 ) {
     tsunami.stopAllTracks(); 
     if (waitForTrackEnd != -1) {
        sendEndOfTrackMessage(waitForTrackEnd);  
     }
  } else if (topic.indexOf("Stop") != -1) {
    int trackNum = message.toInt();
    if (waitForTrackEnd == trackNum) {
      sendEndOfTrackMessage(waitForTrackEnd);  
    }
    tsunami.trackStop(trackNum);
  }
}
