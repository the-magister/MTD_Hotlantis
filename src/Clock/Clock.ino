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

#include <DS3231.h>
#include <Wire.h>

DS3231 Clock;

byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

bool Century = false;
bool h12;
bool PM;

void setup() {
  // for local output
  Serial.begin(115200);
  Serial << endl << endl << endl << "Startup: begin." << endl;

  // Start the I2C interface
  Wire.begin();

  // configure comms
  Comms.begin("gwf/clock", processMessages);

  Serial << "Startup: complete." << endl;
}

void loop() {
  // comms handling
  Comms.loop(false);

  static Metro publishInterval(5000UL);
  if ( publishInterval.check() ) {
    String hour = String( Clock.getHour(h12, PM) );
    String dayofweek = String( Clock.getDoW() );

    // publish new reading.
    Comms.pub(Comms.senseClock[0], hour);
    Comms.pub(Comms.senseClock[1], dayofweek);

    Serial << "Clock. ";
    showTime();

    // once we start up, we can stop frantically trying to publish the time.
    if( Comms.getStatus() == FULL_CONNECTION) publishInterval.interval(60UL * 1000UL);
  }

  // If something is coming in on the serial line, it's
  // a time correction so set the clock accordingly.
  if (Serial.available()) {
    delay(100);
    if( ! GetDateStuff(Year, Month, Date, DoW, Hour, Minute, Second) ) return;

    Clock.setClockMode(false);	// set to 24h
    //setClockMode(true);	// set to 12h

    Clock.setYear(Year);
    Clock.setMonth(Month);
    Clock.setDate(Date);
    Clock.setDoW(DoW);
    Clock.setHour(Hour);
    Clock.setMinute(Minute);
    Clock.setSecond(Second);

    Serial << "clock set: " << endl;
    showTime();
  }
}

void showTime() {
  Serial.print(Clock.getYear(), DEC);
  Serial.print("-");
  Serial.print(Clock.getMonth(Century), DEC);
  Serial.print("-");
  Serial.print(Clock.getDate(), DEC);
  Serial.print(" ");
  Serial.print(Clock.getDoW(), DEC); 
  Serial.print(" ");
  Serial.print(Clock.getHour(h12, PM), DEC); //24-hr
  Serial.print(":");
  Serial.print(Clock.getMinute(), DEC);
  Serial.print(":");
  Serial.println(Clock.getSecond(), DEC);

}

boolean GetDateStuff(byte& Year, byte& Month, byte& Day, byte& DoW,
                  byte& Hour, byte& Minute, byte& Second) {
  // Call this if you notice something coming in on
  // the serial port. The stuff coming in should be in
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean GotString = false;
  char InChar;
  byte Temp1, Temp2;
  char InString[20];

  Metro timeout(1000UL);
  timeout.reset();
  
  byte j = 0;
  while (!GotString) {
    if (Serial.available()) {
      InChar = Serial.read();
      InString[j] = InChar;
      j += 1;
      if (InChar == 'x') {
        GotString = true;
      }
    }
    if( timeout.check() ) {
      Serial << "Enter: YYMMDDwHHMMSSx" << endl;
      return(false); // bail out
    }
  }
  InString[j] = '\0';

  Serial << "Received: " << endl;
  
  Serial.println(InString);
  // Read Year first
  Temp1 = (byte)InString[0] - 48;
  Temp2 = (byte)InString[1] - 48;
  Year = Temp1 * 10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] - 48;
  Temp2 = (byte)InString[3] - 48;
  Month = Temp1 * 10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] - 48;
  Temp2 = (byte)InString[5] - 48;
  Day = Temp1 * 10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;
  // now Hour
  Temp1 = (byte)InString[7] - 48;
  Temp2 = (byte)InString[8] - 48;
  Hour = Temp1 * 10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] - 48;
  Temp2 = (byte)InString[10] - 48;
  Minute = Temp1 * 10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] - 48;
  Temp2 = (byte)InString[12] - 48;
  Second = Temp1 * 10 + Temp2;

  return(true);
}

// processes messages that arrive
void processMessages(String topic, String message) {
}
