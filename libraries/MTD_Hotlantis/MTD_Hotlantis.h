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
	bool begin(String myName, void (*processMessages)(String topic, String message));

	// call this every loop()
	int loop(boolean publishSerialCommands=true);
	
	// publishing sugar
	void pub(String topic, String payload, bool retain=true);
	
	// subscribing sugar.  
	bool sub(String topic);
	
	// topics and messages
	String loadStuff(String key);
	void saveStuff(String key, String value);
	
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
	}; // 0,1
	// Lighting controllers
	const String actMTDLight[3] = { 
		"gwf/a/mtd/A/light", "gwf/a/mtd/B/light", "gwf/a/mtd/C/light"
	}; // String?
	// Fog machine
	const String actMTDFog[1] = { 
		"gwf/a/mtd/fog"
	}; // 0,1

	// Buttons on the water cannon
	const String senseCannon[2] = { 
		"gwf/s/cannon/L", "gwf/s/cannon/R" 
	}; // 0,1
	// Pumps 
	const String actPump[5] = { 
		"gwf/a/pump/prime/A", "gwf/a/pump/prime/B", 
		"gwf/a/pump/boost/A", "gwf/a/pump/boost/B", 
		"gwf/a/pump/cannon"
	}; // 0,1

	// The three navigation beacons for Hotlantis are A, B, C.
	// Water sprayers
	// business end _is_ on the Beacons, but switching is in Pump code
	const String actBeaconSpray[3] = { 
		"gwf/a/beacon/A/spray", "gwf/a/beacon/B/spray", "gwf/a/beacon/C/spray"
	}; // 0-1
	// Flame effects
	const String actBeaconIgniter[3] = { 
		"gwf/a/beacon/A/igniter", "gwf/a/beacon/B/igniter", "gwf/a/beacon/C/igniter"
	}; // 0-1
	const String actBeaconFlame[3] = { 
		"gwf/a/beacon/A/fire", "gwf/a/beacon/B/fire", "gwf/a/beacon/C/fire"
	}; // 0-PWMRANGE 
	// EL wire lighting
	const String actBeaconLight[3] = { 
		"gwf/a/beacon/A/light", "gwf/a/beacon/B/light", "gwf/a/beacon/C/light"
	}; // String?
	
	// Stand-alone stuff
	const String senseClock[2] = { 
		"gwf/s/clock/hour", "gwf/s/clock/dayofweek" 
	}; // both messages are bytes
	const String actSound[1] = { 
		"gwf/a/sound/track"
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
