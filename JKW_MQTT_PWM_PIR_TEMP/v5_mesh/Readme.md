This firmware is the base of my personal smarthome system. The idea is to use the same firmware on a 
variety of devices and let a configuration determine what kind of sensors and services can/will be used.
As of now I have Sonoff Touch, Sonoff Basic, Sonoff Pow and a couple of self build boards in use.

# "Quick" Setup

- Flash firmware and manually reset device after flash (bug in the ESP framework that will cause a reset fail down the road)
- Wait 90 sec
- Connect to the AP (SSID "ESP CONFIG")
- Open http://192.168.4.1
- Configuration the wifi and MQTT settings, assign a unique device short name (e.g. dev01 / dev02 / ... keep this short, it will be used for all mqtt topics in the future)
- Prepare to see the "boot up" message of the device by connecting to the same MQTT broker and subscribe to e.g. "dev01/r/#" (all PC receivable topics of this device)
- Once the device rebooted, it will connect to the configured wifi and publish its default config (e.g. Firmware version etc) which will show up on the PC
- Configure the capabilities of the device by publishing e.g. [mosquito_pub -t "dev01/s/capabilities" -m "R,B,SL" -u MQTT_USER -p MQTT_PW] (see peripherals list below)
- The device will save this data to the EEPROM and reconfigure/reconnect
- Wait for incoming data depending on the configuration (e.g. "dev01/r/RSSI" or "dev01/r/button")

# General concept
The Firmware shall ensure the general base functionalies (WiFi/MQTT) and provide a framework for so called "peripherals".
A peripheral is a class that implemtents sensor specific tasks. As of now there are the following peripherals available:
RSSI, Button, ADC, B1, DHT22, DS18B20, HWL8012, Neopixel, PIR, PWM, RF bridge, Simple light / relay.
See section below for more details and configuration.

### The behavior of the firmware after flashing is defined as following:
- Read configuration from EEPROM (includes WiFi, MQTT, client name, "Capabilities" which is a list of the active peripherals)
- Load the peripherals according to the capability - configuration
- Connect to WiFi
	- without active WiFi
		- Keep searching for the configured WiFi and try to connected for 45 sec (see relationship reconnect)
		- Start searching simultaniously for "Mesh wifi" after 36 sec (see relationship reconnect)
		- Start WiFi Configuration Access Point after 45 sec (see relationship reconnect)
			- A user can connect to this accesspoint with a PC/Tablet/Smartphone
			- Open http://192.168.4.1 and configure base options like SSID and pw for the WiFi as well as the MQTT IP and login
			- The accesspoint will shutdown after 3600 sec. The (re-)connect sequence will start over.
	- On established connection
		- Subscribe to some general topic and publish some general information
		- Let each peripheral subscribe to its own topics
		- Loop
			- Check WiFi connection and reconnect if needed
			- Process incoming MQTT data from the Uplink, e.g. reconfigure WiFi / Capabilities
			- Loop each peripheral e.g. to let them read their sensors data
			- Let each peripheral publish imidiate information whenever they are available, e.g. Button was pushed
			- Publish intervall data of each peripheral once per minute, e.g. Temperature updated
			
# Peripherals supported
### RSSI
	Configuration string: "R"
	Purpose: publishs WiFi strength once per minute
	Sub-Topic(s): "/r/rssi"
	
### Button
	Configuration string: "B"
	Purpose: publishes button push imidiatly, button can be hold which will trigger extra messages
	Sub-Topic(s): "/r/button" / "/r/button1s" / "/r/button2s" / "/r/button3s"
	
### ADC 
	Configuration string: "ADC"
	Purpose: publish raw ADC value once per minute
	Sub-Topic(s): "/s/adc"
	
### Light
	Configuration string: none
	Purpose: will consume all light commands (dimm/color) and forward it to the best available light provider
	Sub-Topic(s):  each subtopic is available as "/s/" and "/r/"  routine (read+write)
		"light":  payload ON / OFF, to switch hard
		"light/color":  payload "0-99,0-99,0-99",  will switch hard to the value
		"light/brightness":  payload "0-99", will switch hard to the value
		"light/dimm":  payload ON/ OFF, to dimm the light on and off
		"light/dimm/brightness":  payload "0-99", will dimm towards the new value
		"light/dimm/delay":  payload "1-255" default 10, eg 10ms per dimm step, 255*0.01 = 2.5 sec 
		"light/dimm/color":  payload "0-99,0-99,0-99", will dimm towards the new value
		"light/animation/brightness":  payload "0-99", will switch hard to the value
		"light/animation/rainbow":  payload ON / OFF, to switch hard
		"light/animation/simple_rainbow":  payload ON / OFF, to switch hard
		"light/animation/color_wipe":  payload ON / OFF, to switch hard

### BOne
	Configuration string: "B1"
	Purpose: will consume all light commands (dimm/color) and forward it the the IC used in the Sonoff B1
	Mentioning the config string "B1" will autoload the light class which takes care of all commands
	Sub-Topic(s): none, is sub-peripheral to light class

### DHT22 
	Configuration string: "DHT"
	Purpose: publishs Temperature and humidity once per minute
	Sub-Topic(s): "/s/temperature" and "/s/humidity"

### DS18B20 
	Configuration string: "DS"
	Purpose: publishs once per minute
	Sub-Topic(s): "/s/temperature"

### HLW8012 
	Configuration string: "HLW"
	Purpose: this chip is used in the Sonoff POW, it publishes Voltage, current and Power once per minute
	Sub-Topic(s): "/s/current" "/s/voltage" "/s/power" 

### Neopixel 
	Configuration string: "NEO"
	Purpose: will consume all light commands (dimm/color) and forward it to a neopixel string
	Sub-Topic(s): none, is sub-peripheral to light class

### PIR 
	Configuration string: "PIR / PI2" (PIR uses input-pin 14, PI2 pin 5)
	Purpose: publishes motion events instantly
	Sub-Topic(s): "/s/motion"

### PWM 
	Configuration string: "PWM / PW2 / PW3" (Pin 4/5/16,0,0    4/4/4,0,0      15,13,12,14,4)
	Purpose: will consume all light commands (dimm/color) and forward it to the selected outputs (R,G,B,W,WW)
	Sub-Topic(s): Sub-Topic(s): none, is sub-peripheral to light class

### RF bridge 
	Configuration string: "RFB"
	Purpose: forwards 433Mhz packages
	Sub-Topic(s): "/s/bridge"

### Simple light / relay 
	Configuration string: "SL"
	Purpose: will consume simple light commands and toggle the pin that is connected to the relay in a Sonoff basic/touch
	Sub-Topic(s): Sub-Topic(s): none, is sub-peripheral to light class


### WiFi Configuration Access Point
Todo

### WiFi Relay feature
Each node that is connected will open an own access point. During (re-)connect phase each device will also try to connect to those access points at some point.
A "mesh connected" (I know that this is not a mesh) device will still send subscriptions on the uplink and also publish messages. The data will be slightly 
converted and send up to the AP. A "mesh server" will receive the message and decode them, if they are not directly send to himself, it will automatically forward the data up the chain.

### Relationship reconnect
