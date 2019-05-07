# Moon Tower Defense and Hotlantis

## Art Project 

### Overview

Atlantean Nautical Beacons illuminating the river as aids to navigation to wayward travelers.  Moon Tower Defense (MTD) is the lighthouse, and the location by which participants interact with the project.

Hotlantis proposes a nighttime extension to Moon Tower Defense that allows participants to act as stewards of a lighthouse and beacons in the night to Atlantean travelers on Canyon Creek.

### Philosophical Statement

Canyon Creek is different by day and night (diurnal).  By day, it’s an exciting play-place; a place to float, cool off, dally.  By night, it’s brooding, inscrutable, noisy and dangerous.  We seek to express how experience varies depending on time of day, point of view, activity level, and community involvement.  Hotlantis is flipside for Moon Tower Defense.  By day, MTD defends when the creek is playful; by night, Hotlantis invites and guides when the creek is shrouded in darkness.

This diurnal feature returns upon sunrise and sundown, as well as constant features. During the day, Hotlantis provides solace with shade, hammocks, and water toys.  At night, the lights come out to play along with a multi-track interactive musical and fire experience that varies based on the number of participants.  We wish to communicate to participants that the experience will be regular, changeable, and replenishing.

## Hardware Environment

MTD/Hotlantis is based on a distributed microcontroller (uC) environment that uses the MQTT protocol for communication.  The uC's are ESP8266-based, generally, which provides a convenient WiFi stack under-the-hood.  A central WiFi router serves as the hub for the project as well as the MQTT broker.  

The uC's have hardware-level connections to peripheral devices that fall into two categories: sensors and actuators.  

### Sensors

Sensors provide input to the system for the purposes of interactivity.  These are:

* [Clock](src/Clock/Clock.ino): day-of-week and time-of-day information from a real-time clock device.
* [Cannon](src/Cannon/Cannon.ino): two buttons on the physical cannon by the shore.
* [Buttons](src/MTDButton/MTDButton.ino): three buttons atop MTD.
* [Motion](src/MTDMotionLight/MTDMotionLight.ino): three passive, infrared (PIR) motion sensors below MTD.

### Actuators

Actuators provide output from the system to generate actions, behaviors and gestures.  These are:

* [Beacon](src/BeaconLightIgniterFlame/BeaconLightIgniterFlame.ino): lighting, pilot igniter, flame
* [Pumps](src/PumpSprinkler/PumpSprinkler.ino): water jet from the cannon, water sprinklers from the Beacons
* [MTD Lighting](src/MTDMotionLight/MTDMotionLight.ino): lighting
* [MTD Fog](src/MTDFog/MTDFog.ino): fog machine
* [Sound](src/Sound/Sound.ind): sound

## Software Environment

MTD/Hotlantis is programmed in C++ within the Arduino IDE.  It is impossible to cleanly delineate where the Electronics and Software functional domains form an interface.  The general idea is that the Electronics group delivers a set of working hardware with a minimal code base that exercises the hardware.  While it is possible to reprogram the uC's in-the-field, remember that these systems could be installed in the middle of a river in a watertight enclosure.  Thus, **it is strongly recommended that the Actuator and Sensor code base be brutally simple and robust, and all complexity reside in a single uC not associated with any hardware**.  This uC is referred to as the **Controller**, and should be the primary target of the Software functional domain.

### MQTT PubSub Topics

The Electronics functional domain has kindly provided a Arduino library, [MTD_Hotlantis](libraries/MTD_Hotlantis/MTD_Hotlantis.h), that operates the WiFi and MQTT stacks automagically.  The main objective should be to:

1. Sensor uC: Read sensors and publish sensor topics.
2. Controller uC: subscribe sensor topics, **integrate sensor information, define activities**, publish actuator topics.
3. Actuator uC: Subscribe actuator topics and act on them.

The MQTT topic hierarchy is as-follows.  See [MTD_Hotlantis.h](libraries/MTD_Hotlantis/MTD_Hotlantis.h) for specifics.

* gwf/	- top level
	* s/	- sensor topics
	* a/	- actuator topics
	




