#ifndef MTD_Hotlantis_h
#define MTD_Hotlantis_h

#include <Arduino.h>
#include <Metro.h>
#include <Streaming.h>

#include <ESPHelper.h>
#include <ESPHelperFS.h>

class MTD_ESPHelper: public ESPHelper {
public:
	MTD_ESPHelper(netInfo *netList[], uint8_t netCount, uint8_t startIndex = 0); // constructor
	
	// startup and store a name for output prefacing.
	bool begin(String myName, void (*processMessages)(String topic, String message), byte heartBeatPin=BUILTIN_LED);

	// call this every loop()
	int loop(boolean publishSerialCommands=true);
	
	// publishing sugar
	void pub(String topic, String payload, bool retain=false);
	
	// subscribing sugar.  
	bool sub(String topic);
	
	// topics and messages
	String loadStuff(String key);
	void saveStuff(String key, String value);

        bool convBinary(char msg);
	
	// MQTT topics, roughly grouped by structure.
	// name is "sense" if it's a sensor the Controller can read (Input)
	// name is "act" if it's an actuator/action the Controller can take (Output)
	
	// The three sides to MTD are A, B, C.
	// Buttons on the top of MTD rails
	const String senseMTDButton[3] = { 
		"gwf/s/mtd/A/button", "gwf/s/mtd/B/button", "gwf/s/mtd/C/button" 
	}; // 0,1
	// Motion sensors under the MTD arches
	const String senseMTDMotion[3] = { 
		"gwf/s/mtd/A/motion", "gwf/s/mtd/B/motion", "gwf/s/mtd/C/motion" 
	}; // 0,1o
	// Lighting controllers
	const String actMTDLight[3] = { 
		"gwf/a/mtd/A/light", "gwf/a/mtd/B/light", "gwf/a/mtd/C/light"
	}; // String?
	// Fog machine
	const String actMTDFogBubbleEtc[9] = { 
		"gwf/a/mtd/fog", // 0,1
		"gwf/a/mtd/bubble", // 0,1
		// these might get hooked up to 110VAC devices.
		"gwf/a/mtd/chan2","gwf/a/mtd/chan3","gwf/a/mtd/chan4", // 0,1
		"gwf/a/mtd/spotA","gwf/a/mtd/spotB","gwf/a/mtd/spotC","gwf/a/mtd/spotD" // hue:0,255
	}; 
	
	// Buttons on the water cannon
	const String senseCannon[2] = { 
		"gwf/s/cannon/L", "gwf/s/cannon/R" 
	}; // 0,1
	// A/pump is a box, with 2 pump controls (A,B) and 4 solenoid controls (A,B,C,D)
	// B/pump is a duplicate
	// Pumps 
	const String actPumpPump[4] = { 
		"gwf/a/pump/A/pump/A", "gwf/a/pump/A/pump/B", 
		"gwf/a/pump/B/pump/A", "gwf/a/pump/B/pump/B", 
	}; // 0,1
	// Water sprayers
	// business end _is_ on the Beacons, but switching is in Pump code
	const String actPumpSpray[8] = { 
		"gwf/a/pump/A/spray/A", "gwf/a/pump/A/spray/B", "gwf/a/pump/A/spray/C", "gwf/a/pump/A/spray/D",
		"gwf/a/pump/B/spray/A", "gwf/a/pump/B/spray/B", "gwf/a/pump/B/spray/C", "gwf/a/pump/B/spray/D"
	}; // 0,1
	
	// The three navigation beacons for Hotlantis are A, B, C.
	// Flame effects
	const String actBeaconIgniter[3] = { 
		"gwf/a/beacon/A/igniter", "gwf/a/beacon/B/igniter", "gwf/a/beacon/C/igniter"
	}; // 0-1
	const String actBeaconFlame[8] = { 
		// Send the flame effect level.  Values in 0..pwmrange
		"gwf/a/beacon/A/fire", 
		"gwf/a/beacon/B/fire", 
		"gwf/a/beacon/C/fire", 
		// set the granularity of the ADC.  1023 default.
		"gwf/a/beacon/pwmrange", 
		// set the frequency for solenoid operation. 300 Hz default.
		"gwf/a/beacon/pwmfreq", 
		// ramps are used to smooth transitions.  See Ramp.h for options.
		// set the time it takes to change between /fire values; 5000ms default
		"gwf/a/beacon/ramptime",
		// set the interpolation mode for ramp; EXPONENTIAL_OUT default. may want QUADRATIC_OUT, CUBIC_OUT to get more flow at the lower end of the pwmrange
		"gwf/a/beacon/rampmode",
		// set the loop for ramp; ONCEFORWARD default. may want FORTHANDBACK for cycling between the last two values.
		"gwf/a/beacon/ramploop"
	}; 
	// EL wire lighting
	const String actBeaconLight[3] = { 
		"gwf/a/beacon/A/light", "gwf/a/beacon/B/light", "gwf/a/beacon/C/light"
	};	// String, if we go with LED lights. 
		// 0-PWMRANGE, if we go with EL lights.  Also append "/A".."/F" in to the topic
	
	// Stand-alone stuff
	const String senseClock[2] = { 
		"gwf/s/clock/hour", "gwf/s/clock/dayofweek" 
	}; // both messages are bytes
	const String actSound[1] = { 
		"gwf/a/sound/"
		// MGD: I think this is a bad idea, and the messages should be enumerated here
		// in this header file.  If you do that, Sound.ino can look at this file
		// and implement all of these messages, and Controller.ino can look at this 
		// file and understand what the "menu" of gestures are.
		// If you bypass this header, then one must look at both source files at
		// the same time.  And a _typo_ can completely eff everything up.  
		// Basically: message _contents_ can be highly variable, but I'd recommend
		// making the _topics_ deterministic.  
		// I mean:
		// "gwf/a/sound/Play", "gwf/a/sound/PlaySolo", etc.
	}; // both messages are bytes
	
	
	// You can send whatever you like, serialized as a string.
	const String messageBinary[2] = { 
		"0",	// off
		"1" 	// on
	};
	
private:
	String myName,OTAname;
	
	void (*processMessages)(String topic, String message);
	
	void MQTTcallback(char* topic, byte* payload, unsigned int length);
	
	byte nSubTopic = 0;
	String subTopics[MAX_SUBSCRIPTIONS];
};

extern MTD_ESPHelper Comms;

// helper
#define ENTRIESIN(object) (sizeof(object)/sizeof(object[0]))

#endif

/* 

GPIO pins: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

Label	GPIO	Input	Output	Notes
D0		GPIO16	no interrupt	no PWM or I2C support; HIGH at boot; used to wake up from deep sleep
D1		GPIO5	OK	OK	often used as SCL (I2C)
D2		GPIO4	OK	OK	often used as SDA (I2C)
D3		GPIO0	pulled up	OK	connected to FLASH button, boot fails if pulled LOW
D4		GPIO2	pulled up	OK	HIGH at boot; connected to on-board LED, boot fails if pulled LOW
D5		GPIO14	OK	OK	SPI (SCLK)
D6		GPIO12	OK	OK	SPI (MISO)
D7		GPIO13	OK	OK	SPI (MOSI)
D8		GPIO15	pulled to GND	OK	SPI (CS) Boot fails if pulled HIGH
RX		GPIO3	OK	RX pin	HIGH at boot
TX		GPIO1	TX pin	OK	HIGH at boot; debug output at boot, boot fails if pulled LOW
A0		ADC0	Analog Input	X	

*/
