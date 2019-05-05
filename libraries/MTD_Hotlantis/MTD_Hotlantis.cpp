#include "MTD_Hotlantis.h"

netInfo fieldNet = {
  .mqttHost = "192.168.4.1",     //can be blank if not using MQTT
  .mqttUser = "",   //can be blank
  .mqttPass = "",   //can be blank
  .mqttPort = 1883,   //default port for MQTT is 1883 - only change if needed.
  .ssid = "GamesWithFire",
  .pass = "safetythird"
};
netInfo homeNet = {
  .mqttHost = "m2m.eclipse.org",     //can be blank if not using MQTT
//  .mqttHost = "test.mosquitto.org",     //can be blank if not using MQTT
  .mqttUser = "",   //can be blank
  .mqttPass = "",   //can be blank
  .mqttPort = 1883, //default port for MQTT is 1883 - only change if needed.
  .ssid = "Looney",
  .pass = "TinyandTooney"
};
netInfo *knownNetworks[2] = { &fieldNet, &homeNet };

MTD_ESPHelper Comms(knownNetworks, 2, 1); // set to 2, 0 in production
// MTD_ESPHelper Comms(knownNetworks, 2, 0); 

bool MTD_ESPHelper::begin( String name, void (*processMessages)(String topic, String message) ) {
	this->myName = name;
	this->processMessages = processMessages;

	// holy shit.  this was hard.
	ESPHelper::setMQTTCallback(
		[this] (char* topic, byte* payload, unsigned int length) { 
			this->MQTTcallback(topic, payload, length); 
		}
	);

	// blinky
	pinMode(BUILTIN_LED, OUTPUT);
	ESPHelper::enableHeartbeat(BUILTIN_LED);

	// process Serial commands
	Serial.setTimeout(100);
	
	return( ESPHelper::begin() );
}

int MTD_ESPHelper::loop(boolean publishSerialCommands) {
	// add some riders, as-needed
	
	// heartbeat
	static Metro heartBeat(60000UL);
	if( heartBeat.check() ) {
		Serial << this->myName << ". comms: ";
		switch( ESPHelper::getStatus() ) {
			case NO_CONNECTION: Serial << "-wifi -mqtt"; break;
			case BROADCAST: Serial << "BROADCAST wtf?"; break;
			case WIFI_ONLY: Serial << "+WiFi -mqtt"; break;
			case FULL_CONNECTION: Serial << "+WiFi +MQTT"; break;
		}
		Serial << " at " << millis()/1000/60 << " min." << endl;      
	}

	// serial command?
	if( publishSerialCommands && Serial.available() ) {
		
		String in = Serial.readStringUntil('\0');
		Serial << "Serial received: [" << in << "].  ";
		
		int split = in.indexOf('=');
		if( split == - 1) {
			Serial << "Parse error.  Enter <topic>=<value>, e.g. nyc/mtd/rail/A=1" << endl;
		} else {
			Serial << "Parsed successfully." << endl;
			// split
			String topic = in.substring(0,split);
			String message = in.substring(split+1);
			// send, but not retained
			this->pub(topic, message, false);
		}
	}
	
	// call for fundamental activity
	
	return( ESPHelper::loop() );
}


void MTD_ESPHelper::pub(String topic, String payload, bool retain) {
	Serial << this->myName;
	
	if( ESPHelper::getStatus() == FULL_CONNECTION ) {
		// send
		ESPHelper::publish(topic.c_str(), payload.c_str(), retain);
		ESPHelper::heartbeat();
		Serial << " --> [";
	} else {
		// can't send
		Serial << " !X! [";
	}
	
	Serial << topic << "]=[" << payload << "]" << endl;
}

/*
This code is super fucking dangerous.
bool MTD_ESPHelper::sub(String topic) {
	Serial << this->myName << ". subscribing:" << topic << endl;
	return( ESPHelper::addSubscription(topic.c_str()));
}
*/

const char* FSfile = "/mtd.json";
String MTD_ESPHelper::loadStuff(String key) {
    ESPHelperFS::begin();
    String ret = ESPHelperFS::loadKey(key.c_str(), FSfile);
    ESPHelperFS::end();	
	
	return(ret);
}
void MTD_ESPHelper::saveStuff(String key, String value) {
    ESPHelperFS::begin();
    ESPHelperFS::addKey(key.c_str(), value.c_str(), FSfile);
    ESPHelperFS::end();
}

void MTD_ESPHelper::MQTTcallback(char* topic, byte* payload, unsigned int length) {
	// String class is much easier to work with
	payload[length] = '\0';
	String t = topic;
	String p = String((char *)payload);
	
    Serial << this->myName << " <-- [" << t << "]=[" << p << "]" << endl;
	this->processMessages(t, p);
}

// constructor plumbing
MTD_ESPHelper::MTD_ESPHelper(netInfo *netList[], uint8_t netCount, uint8_t startIndex): ESPHelper(netList, netCount, startIndex) {}

