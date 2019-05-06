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
		"gwf/mtd/A/button", "gwf/mtd/B/button", "gwf/mtd/C/button" 
	};
	// Motion sensors under the MTD arches
	const String senseMTDMotion[3] = { 
		"gwf/mtd/A/motion", "gwf/mtd/B/motion", "gwf/mtd/C/motion" 
	};
	// Lighting controllers
	const String actMTDLight[3] = { 
		"gwf/mtd/A/light", "gwf/mtd/B/light", "gwf/mtd/C/light"
	};
	// Fog machine
	const String actMTDFog[1] = { 
		"gwf/mtd/fog"
	};

	// Buttons on the water cannon
	const String senseCannon[2] = { 
		"gwf/cannon/L", "gwf/cannon/R" 
	};
	// Pumps 
	const String actPump[5] = { 
		"gwf/pump/prime/A", "gwf/pump/prime/B", 
		"gwf/pump/boost/A", "gwf/pump/boost/B", 
		"gwf/pump/cannon"
	};

	// The three navigation beacons for Hotlantis are A, B, C.
	// Water sprayers
	// business end _is_ on the Beacons, but switching is in Pump code
	const String actBeaconSpray[3] = { 
		"gwf/beacon/A/spray", "gwf/beacon/B/spray", "gwf/beacon/C/spray"
	};
	// Flame effects
	const String actBeaconIgniter[3] = { 
		"gwf/beacon/A/igniter", "gwf/beacon/B/igniter", "gwf/beacon/C/igniter"
	};
	const String actBeaconFlame[3] = { 
		"gwf/beacon/A/fire", "gwf/beacon/B/fire", "gwf/beacon/C/fire"
	};
	// EL wire lighting
	const String actBeaconLight[3] = { 
		"gwf/beacon/A/light", "gwf/beacon/B/light", "gwf/beacon/C/light"
	};
	
	// Stand-alone stuff
	const String senseClock[2] = { 
		"gwf/clock/hour", "gwf/clock/dayofweek" 
	};
	// both messageClock are bytes
	
	// You can send whatever you like, serialized as a string.
	const String messageBinary[2] = { 
		"0",	// off
		"1" 	// on
	};
	
private:
	String myName;
	
	void (*processMessages)(String topic, String message);
	
	void MQTTcallback(char* topic, byte* payload, unsigned int length);
	
	byte nSubTopic = 0;
	String subTopics[MAX_SUBSCRIPTIONS];
};

extern MTD_ESPHelper Comms;

// helper
#define ENTRIESIN(object) (sizeof(object)/sizeof(object[0]))

#endif

