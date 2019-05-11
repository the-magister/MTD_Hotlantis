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
#define PIN_BUTTON D1 // wire D3/GPIO0 through a N.O. switch to GND
Bounce button = Bounce();
String buttonTopic;

void setup() {
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // enable pin.
  button.attach(PIN_BUTTON, INPUT_PULLUP);
  button.interval(5); // interval in ms

  // bootstrap new microcontrollers, if needed.
  if ( true ) {
    Comms.saveStuff("publishTopic", Comms.senseMTDButton[0]);
  }
  buttonTopic = Comms.loadStuff("publishTopic");
  Serial << "Startup: publishing to topic [" << buttonTopic << "]." << endl;

  // configure comms
  Comms.begin(buttonTopic, processMessages);

  Serial << "Startup: complete." << endl;
}

void loop() {
  // comms handling
  Comms.loop();

  // update buttons
  if ( button.update() ) {
    // publish new reading.
    Comms.pub(buttonTopic, Comms.messageBinary[button.read() == onReading]);
  }
}

// processes messages that arrive
void processMessages(String topic, String message) {
}
