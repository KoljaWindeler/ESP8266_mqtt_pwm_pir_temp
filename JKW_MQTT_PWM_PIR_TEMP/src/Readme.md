This firmware is the base of my personal smarthome system. The idea is to use the same firmware on a
variety of devices and let a configuration determine what kind of sensors and services can/will be used.
As of now I have Sonoff Touch, Sonoff Basic, Sonoff Pow and a couple of self build boards in use.

# "Quick" Setup

- Flash firmware and manually reset device after flash (bug in the ESP framework that will cause a reset fail down the road)
- Wait 90 sec
- Connect to the AP (SSID "ESP CONFIG")
- Open http://192.168.4.1
- Configuration the wifi and MQTT settings, assign a unique device short name (e.g. "dev01" or even a location like "house/livingroom/tower" ... limit this to 50 chars, it will be used for all mqtt topics in the future
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
All sub topic will concatenated with the dev_short and the direction: e.g. "dev99/r/rssi"

### RSSI
	Configuration string: "R"
	Purpose: publishs WiFi strength once per minute
	Sub-Topic(s): "rssi" (out only)

### Button
	Configuration string: "B", "B*", "BP*", "BN*","BS*", "BSP*", "BSN*",  with * = {0..5, 12..16}
	Purpose: push button (B{,P,N}*) or switch (BS{,P,N}*), any change will imidiatly toggle the light
	(as long as a light provider is configured). Publishes also imidiatly a message.
	Push button can be hold which will trigger extra messages.
	Sub-Topic(s): "button" / "button1s" / "button2s" / "button3s" (all out only)

### ADC
	Configuration string: "ADC"
	Purpose: publish raw ADC value once per minute
	Sub-Topic(s): "adc" (out only)

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

### Ai
	Configuration string: "AI"
	Purpose: will consume all light commands (dimm/color) and forward it the the IC used in the Thinker AI
	Mentioning the config string "AI" will autoload the light class which takes care of all commands
	Sub-Topic(s): none, is sub-peripheral to light class

### PWM
	Configuration string: "PWM / PW2 / PW3" (Pin 4/5/16,0,0    4/4/4,0,0      15,13,12,14,4)
	Purpose: will consume all light commands (dimm/color) and forward it to the selected outputs (R,G,B,W,WW)
	Sub-Topic(s): Sub-Topic(s): none, is sub-peripheral to light class

### Neopixel
	Configuration string: "NEO"
	Purpose: will consume all light commands (dimm/color) and forward it to a neopixel string
	Sub-Topic(s): none, is sub-peripheral to light class
	
### Simple light / relay
	Configuration string: "SL"
	Purpose: will consume simple light commands and toggle the pin that is connected to the relay in a Sonoff basic/touch
	Sub-Topic(s): Sub-Topic(s): none, is sub-peripheral to light class

### DHT22
	Configuration string: "DHT"
	Purpose: publishs Temperature and humidity once per minute
	Sub-Topic(s): "temperature" and "humidity" (both out only)

### DS18B20
	Configuration string: "DS"
	Purpose: publishs once per minute
	Sub-Topic(s): "temperature" (out only)

### HLW8012
	Configuration string: "HLW"
	Purpose: this chip is used in the Sonoff POW, it publishes Voltage, current and Power once per minute
	Sub-Topic(s): "current" (out only) "/s/voltage" (out only) "/s/power" (out only)

### PIR
	Configuration string: "PIR / PI2" (PIR uses input-pin 14, PI2 pin 5)
	Purpose: publishes motion events instantly
	Sub-Topic(s): "motion"

### RF bridge
	Configuration string: "RFB"
	Purpose: forwards 433Mhz packages
	Sub-Topic(s): "bridge"

### GPIO
	Configuration string: "G[Polarity: {P/N} ][Direction: {I/O}][GPIO Pin: {0..16}]"
	e.g. "GPI1" / "GNI3" / "GNO5"  / "GPO16"

	Purpose: directly set or get pin out-/input, watch out: this module can override other outputs
	GPO4 will configure GPIO4 (! NOT D4, GPIO4 !) as an output with positive polarity ("ON" will be "HIGH)
	GNI3 will configure GPIO3 as LOW-active Input, thus publishing "ON" when the pin goes LOW
	GPIO 6-11 are not available due to their connection to the flash. Usage of those will be ignored.

	Sub-Topic(s) if used as INPUT:
		"gpio_0_state"
			will publish ON/OFF on this topic
		"gpio_0_hold"
			will report 1,2,3,..,9 if the input is ON for the same amount of time [sec]

	Sub-Topic(s) if used as OUTPUT:
		"gpio_0_state"
			publish "ON" / "OFF" or the int value {0..99} to e.g. dev4/s/gpio_0_out
			to set the PIN (0..99 will set the PWM
			to 0..99% durty cycle) if used as output
		"gpio_0_toggle"
			no payload, will toggle between ON/OFF
		"gpio_0_pulse"
			payload is pulse time in ms, will set pin ON and back to OFF after xx ms
		"gpio_0_dimm"
			will sweep the PWM "ON"/"OFF", like dimming a light ON and OFF
		"gpio_0_brightness"
			max PWM level that is used during dimm command, make this a retained msg

	HomeAssistant PWM light config:
		light:
		  - platform: mqtt
		    name: "test_esp"
		    state_topic: "test_esp/r/gpio_2_state"
		    command_topic: "test_esp/s/gpio_2_dimm"
		    brightness_state_topic: "test_esp/r/gpio_2_brightness"
		    brightness_command_topic: "test_esp/s/gpio_2_brightness"
		    retain: True
		    qos: 0
		    payload_on: "ON"
		    payload_off: "OFF"
		    optimistic: false
		    brightness_scale: 99

### Audio
	Configuration string: "AUD"
	Purpose: will provide a server (port 5522) that consumes special audio streams (send by a Raspberry). Output is the RX pin.
	Sub-Topic(s): Sub-Topic(s): none, consumes direct streams
	
### Frequency counter
	Configuration string: "FRE"
	Purpose: Counts pin toggles on a definable GPIO and reports them once a minute. E.g. count kw/h via LED pulse
	Sub-Topic(s): "freq"

### Husqvarna
	Configuration string: "HUS"
	Purpose: Can controll a husqvarna automover via soft serial. This is untested.
	Sub-Topic(s): "husqvarna_mode_[...]"
	
### Night light
	Configuration string: "NL"
	Purpose: Sets the mini LED on the SonOff Touch, subscribs to a global non device specific topic
	Sub-Topic(s): "nightlight"

### Uptime
	Configuration string: "UT"
	Purpose: Counts the time that a pin is in a defined state (e.g. counts seconds of "there is rain" from my rain sensor)
	Sub-Topic(s): "uptime"
	

### WiFi Configuration Access Point
Todo, but basically a copy of the WiFiManager. Enhanced with some mqtt data saving / loading.

### WiFi Relay feature
Each node that is connected will open an own access point. During (re-)connect phase each device will also try to connect to those access points at some point. A "mesh connected" (I know that this is not a mesh) device will still send subscriptions on the uplink and also publish messages. The data will be slightly converted and send up to the AP. A "mesh server" will receive the message and decode them, if they are not directly send to himself, it will automatically forward the data up the chain.

### Relationship timeout
A client that wasn't connected to the MQTT server (since reboot) should "pretty quickly" activate its own configuration AP.
But a client that was connected to the MQTT server for a long time should try a little longer to reconnect to the server before starting the configuration AP.

The "relactionship timeout" give you both: Initally the time to start the AP is set to 45 sec. Each second that the client is
connected to the MQTT will increase the time before the configuration AP will be started if the client gets disconnected.
The client will try 1 second longer to reconnect per 20 sec of previous connection time. The maximum time is limited to 20 minutes.

The MESH mode will kick in after 80% of the waittime hast passed.

# Tricks
All devices will publish a set of information on connect (helps to check if the device connected):
1) dev97/r/INFO SONOFF 20180413  	--> firmware version
2) dev97/r/SSID JKW_relay_dev23_L1 	--> SSID of the connected AP
3) dev97/r/BSSID c5:60:8e:94:01:62	--> BSSID of the connected AP
4) dev97/r/MAC 26:20:22:e8:3a:2c	--> own MAC
5) dev97/r/capability GIN4,GOP2		--> configured capabilityes
6) dev97/r/routing dev97 >> dev23	--> routing info, this is valueable for MESH mode

All devices will also subscribe to a set of topics:
1) dev95/s/trace
	- "ON"/"OFF" as payload, once activated all debug output will be published at dev95/r/trace
2) dev95/s/setup
	- "ON" start WiFi access point for confirguration
	- "http://192.168.2.84:81/20180403.bin" will update the firmware from the given URL
	- "reset" reboot the ESP
	- "setNW/[SSID]/[pw]/[mqtt server ip]/CHK" CHK is a single byte = xor of SSID,PW,IP
	    to calculate CHK: connect via serial to a ESP and send msg with random CHK, debug will show expected CHK
3) dev95/s/capability
	- "B,PIR,PWM" config string as mentioned above
4) dev95/s/ota
	- MQTT firmware update .. not stable ATM
5) s/ota
	- MQTT firmware update .. not stable ATM
