// IDE Settings:
// Tools->Board : "WeMos D1 R2 & mini"
// Tools->Flash Size : "4M (1M SPIFFS)"
// Tools->CPU Frequency : "160 MHz"
// Tools->Upload Speed : "921600"

#include <Streaming.h>
#include <Metro.h>
#include <Bounce2.h>
#include <ESPHelper.h>
#include <ESPHelperFS.h>
#include <MTD_Hotlantis.h>

// wire it up
const boolean onReading = false;
// devices with the light shield block access to D5-D8
#define PIN_BUTTON_A D1 // wire D3/GPIO0 through a N.O. switch to GND
#define PIN_BUTTON_B D2 // wire D3/GPIO0 through a N.O. switch to GND
#define PIN_BUTTON_C D3 // wire D3/GPIO0 through a N.O. switch to GND
Bounce buttonA = Bounce();
Bounce buttonB = Bounce();
Bounce buttonC = Bounce();

void setup() {
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // enable pin.
  buttonA.attach(PIN_BUTTON_A, INPUT_PULLUP);
  buttonA.interval(5); // interval in ms
  buttonB.attach(PIN_BUTTON_B, INPUT_PULLUP);
  buttonB.interval(5); // interval in ms
  buttonC.attach(PIN_BUTTON_C, INPUT_PULLUP);
  buttonC.interval(5); // interval in ms

  // bootstrap new microcontrollers, if needed.
//  Comms.saveStuff("publishTopic", Comms.senseMTDButton[2]);

  Serial << "Subbing to topics" << endl;
  
  // configure comms
  Comms.begin("ButtonFaker", processMessages);

  Serial << "Startup: complete." << endl;
}

void loop() {
  // comms handling
  Comms.loop();

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

// processes messages that arrive
void processMessages(String topic, String message) {
}
