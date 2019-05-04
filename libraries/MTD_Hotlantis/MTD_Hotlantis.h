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
	int loop();
	
	// publishing sugar
	void pub(String topic, String payload, bool retain=true);
	
	// subscribing sugar
//	bool sub(const String topic);
	
	// topics and messages
	String loadTopic(String key);
	void saveTopic(String key, String value);
	
	// overall 
	const String topicStatus[1] = { 
		"nyc/status" 
	};
	const String messageStatus[2] = { 
		"idle", 
		"on" 
	};
	
	// clock
	const String topicClock[2] = { 
		"nyc/clock/hour", "nyc/clock/dayofweek" 
	};
	// both messageClock are bytes

	// binary sensors
	const String topicRail[3] = { 
		"nyc/mtd/rail/A", "nyc/mtd/rail/B", "nyc/mtd/rail/C" 
	};
	const String topicMotion[3] = { 
		"nyc/mtd/motion/A", "nyc/mtd/motion/B", "nyc/mtd/motion/C" 
	};
	const String topicCannon[2] = { 
		"nyc/cannon/L", "nyc/cannon/R" 
	};
	const String messageBinary[2] = { 
		"0",	// off
		"1" 	// on
	};

	// lighting and fire outputs
	const String topicInteraction[3] = { 
		"nyc/interaction/A", "nyc/interaction/B", "nyc/interaction/C" 
	};
	const String messageInteraction[7] = { 
		"idle", 		// no interaction with you
		"ohai", 		// interaction with you is initiated
		"you", 			// interaction is localized to your area
		"you+left", 	// interaction is localized to your area and leftward area
		"you+right", 	// interaction is localized to your area and rightward area
		"allyall", 		// interaction is pervasive
		"fanfare" 		// go nuts
	};
	
private:

	String myName;
	
	void (*processMessages)(String topic, String message);
	
	void MQTTcallback(char* topic, byte* payload, unsigned int length);
};

extern MTD_ESPHelper Comms;

// helper
#define ENTRIESIN(object) (sizeof(object)/sizeof(object[0]))

#endif

