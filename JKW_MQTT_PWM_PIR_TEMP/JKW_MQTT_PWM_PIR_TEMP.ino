/*
	Highly based on a combination of different version of 
	https://github.com/mertenats/Open-Home-Automation/tree/master/ha_mqtt_binary_sensor_pir
	

	Configuration (HA) : 
	  light:
	    platform: mqtt
	    name: 'Office light'
	    state_topic: 'office/light'
	    command_topic: 'office/light/switch'
	    brightness_state_topic: 'office/light/brightness'
	    brightness_command_topic: 'office/light/brightness/set'
	    optimistic: false

   	Kolja Windeler v0.1 - untested
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h> 


#define MQTT_VERSION MQTT_VERSION_3_1_1
#define DEV "dev1"
#define DIMM_DONE 	0
#define DIMM_DIMMING 	1


// Wifi: SSID and password
#include "wifi.h"

// MQTT: ID, server IP, port, username and password
const PROGMEM char*     MQTT_CLIENT_ID    			= "office_light";
const PROGMEM char*     MQTT_SERVER_IP    			= "192.168.2.84";
const PROGMEM uint16_t  MQTT_SERVER_PORT  			= 1883;
const PROGMEM char*     MQTT_USER         			= "ha";
const PROGMEM char*     MQTT_PASSWORD     			= "ah";

// MQTT: topics
const PROGMEM char*     MQTT_PWM_LIGHT_STATE_TOPIC		= DEV "/PWM_light/status";		// publish state here
const PROGMEM char*     MQTT_PWM_LIGHT_COMMAND_TOPIC		= DEV "/PWM_light/switch"; 		// get command here

const PROGMEM char*     MQTT_SIMPLE_LIGHT_STATE_TOPIC		= DEV "/simple_light";			// publish state here
const PROGMEM char*     MQTT_SIMPLE_LIGHT_COMMAND_TOPIC		= DEV "/simple_light/switch"; 		// get command here

const PROGMEM char* 	  MQTT_MOTION_STATUS_TOPIC		= DEV "/motion/status";			// publish
const PROGMEM char* 	  MQTT_TEMPARATURE_TOPIC			= DEV "/temperature";			// publish
const PROGMEM char* 	  MQTT_HUMIDITY_TOPIC			= DEV "/humidity";			// publish

const PROGMEM char*     MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC   = DEV "/PWM_light/brightness";		// publish
const PROGMEM char*     MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC = DEV "/PWM_light/brightness/set";	// set value
const PROGMEM char*     MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC 	= DEV "/PWM_dimm/delay/set";		// set value
const PROGMEM char*     MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC 	= DEV "/PWM_dimm/brightness/set";	// set value

const PROGMEM char*     MQTT_RSSI_STATE_TOPIC   = DEV "/rssi";    // publish

// payloads by default (on/off)
const PROGMEM char*     STATE_ON          			= "ON";
const PROGMEM char*     STATE_OFF         			= "OFF";

// variables used to store the statea and the brightness of the light
boolean 		m_pwm_light_state 			          = false;
boolean     m_published_pwm_light_state       = true; //testwise
boolean			m_simple_light_state 		          = false;
boolean     m_published_simple_light_state    = true; //testwise
uint8_t 		m_light_brightness			          = 255;
uint8_t     m_published_light_brightness      = 0; //testwise

uint16_t		m_pwm_dimm_time				            = 100; // 100ms per Step, 255*0.1 = 25 sec
uint8_t			m_pwm_dimm_state			            = DIMM_DONE;
uint8_t			m_pwm_dimm_target			            = 0;

// variables used to store the pir state
uint8_t 		m_pir_state 				              = LOW; // no motion detected
uint8_t 		m_published_pir_state             = LOW;
uint8_t 		m_pir_value 				              = 0;


// pin used for the led (PWM)
#define DHT 		1
#define DS 		2
#define DHT_DS_MODE 	DS

#define PWM_LIGHT_PIN 		2 // IC pin 16
#define SIMPLE_LIGHT_PIN 	4 // IC pin 12 --> 7 crashes the MCU
#define BUTTON_INPUT_PIN 	3 // IC pin 15
#define DHT_PIN 		5 // IC pin 13 ... nicht auf der mcu aber gut am sonoff .. hmm
#define PIR_PIN 		9 // IC pin 24 ---> 7,6,8,1 crashes, 9 works

// buffer used to send/receive data with MQTT
const uint8_t 		MSG_BUFFER_SIZE 	= 60;
char 			m_msg_buffer[MSG_BUFFER_SIZE];

WiFiClient 		wifiClient;
PubSubClient 		client(wifiClient);

//Temperature chip i/o
#if DHT_DS_MODE == DHT
	#include "DHT.h"
	DHT 		dht(DHT_PIN, DHT22); // DHT22
#else
	OneWire 	ds(DHT_PIN);  // on digital pin DHT_PIN
#endif

#define UPDATE_TEMP 60000 // once per minute
uint32_t 		timer		= 0;
uint32_t 		timer_dimmer	= 0;

//////////////////////////////////// PUBLISHER ///////////////////////////////////////
// function called to publish the state of the led (on/off)
void publishPWMLightState() {
  Serial.println("publish PWM state");
	if (m_pwm_light_state) {
		client.publish(MQTT_PWM_LIGHT_STATE_TOPIC, STATE_ON, true);
	} else {
		client.publish(MQTT_PWM_LIGHT_STATE_TOPIC, STATE_OFF, true);
	}
  m_published_pwm_light_state = m_pwm_light_state;
}

// function called to publish the brightness of the led
void publishPWMLightBrightness() {
	Serial.println("publish PWM brightness");
	snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_light_brightness);
	client.publish(MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC, m_msg_buffer, true);
  m_published_light_brightness = m_light_brightness;
}


// function called to publish the state of the led (on/off)
void publishSimpleLightState() {
  Serial.println("publish simple light state");
	if (m_simple_light_state) {
		client.publish(MQTT_SIMPLE_LIGHT_STATE_TOPIC, STATE_ON, true);
	} else {
		client.publish(MQTT_SIMPLE_LIGHT_STATE_TOPIC, STATE_OFF, true);
	}
  m_published_simple_light_state = m_simple_light_state;
}

// function called to publish the state of the PIR (on/off)
void publishPirState() {
  Serial.println("publish pir state");
	if (m_pir_state) {
		client.publish(MQTT_MOTION_STATUS_TOPIC, STATE_ON, true);
	} else {
		client.publish(MQTT_MOTION_STATUS_TOPIC, STATE_OFF, true);
	}
	m_published_pir_state = m_pir_state;
}

// function called to publish the brightness of the led
void publishTemperature(float temp) {
  Serial.println("publish temp");
	dtostrf(temp, 3, 2, m_msg_buffer);
	client.publish(MQTT_TEMPARATURE_TOPIC, m_msg_buffer, true);
}

#if DHT_DS_MODE == DHT 
// function called to publish the brightness of the led
void publishHumidity(float hum) {
  Serial.println("publish humidiy");
	dtostrf(hum, 3, 2, m_msg_buffer);
	client.publish(MQTT_HUMIDITY_TOPIC, m_msg_buffer, true);
}
#endif

void publishRssi(float rssi) {
  Serial.println("publish rssi");
  dtostrf(rssi, 3, 2, m_msg_buffer);
  client.publish(MQTT_RSSI_STATE_TOPIC, m_msg_buffer, true);
}
//////////////////////////////////// PUBLISHER ///////////////////////////////////////


///////////////////// function called when a MQTT message arrived /////////////////////
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
	// concat the payload into a string
	String payload;
	for (uint8_t i = 0; i < p_length; i++) {
		payload.concat((char)p_payload[i]);
	}
	// handle message topic
	////////////////////////// SET LIGHT ON/OFF ////////////////////////
	if (String(MQTT_PWM_LIGHT_COMMAND_TOPIC).equals(p_topic)) {
		// test if the payload is equal to "ON" or "OFF"
		if (payload.equals(String(STATE_ON))) {
			if (m_pwm_light_state != true) {
				m_pwm_light_state = true;
				setPWMLightState();
			}
		} else if (payload.equals(String(STATE_OFF))) {
			if (m_pwm_light_state != false) {
				m_pwm_light_state = false;
				setPWMLightState();
			}
		}
	}
	else if (String(MQTT_SIMPLE_LIGHT_COMMAND_TOPIC).equals(p_topic)) {
		// test if the payload is equal to "ON" or "OFF"
		if (payload.equals(String(STATE_ON))) {
			if (m_simple_light_state != true) {
				m_simple_light_state = true;
				setSimpleLightState();
			}
		} else if (payload.equals(String(STATE_OFF))) {
			if (m_simple_light_state != false) {
				m_simple_light_state = false;
				setSimpleLightState();
			}
		}
	}
	////////////////////////// SET LIGHT ON/OFF ////////////////////////
	////////////////////////// SET LIGHT BRIGHTNESS ////////////////////////
	else if (String(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC).equals(p_topic)) {
		uint8_t brightness = payload.toInt();
		if (brightness < 0 || brightness > 255) {
			// do nothing...
			return;
		} else {
			m_light_brightness = brightness;
			setPWMLightState();		
		}
	}
	else if (String(MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC).equals(p_topic)) {
		uint8_t brightness = payload.toInt();
		if (brightness < 0 || brightness > 255) {
			// do nothing...
			return;
		} else {
			pwmDimmTo(brightness);
		}
	}
	else if (String(MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC).equals(p_topic)) {
		m_pwm_dimm_time = payload.toInt();
	}
	////////////////////////// SET LIGHT BRIGHTNESS ////////////////////////
}
///////////////////// function called when a MQTT message arrived /////////////////////

////////////////////// peripheral function ///////////////////////////////////

// function called to adapt the brightness and the state of the led
void setPWMLightState() {
	if (m_pwm_light_state) {
		analogWrite(PWM_LIGHT_PIN, m_light_brightness);
		Serial.print("INFO: PWM Brightness: ");
		Serial.println(m_light_brightness);
	} else {
		analogWrite(PWM_LIGHT_PIN, 0);
		Serial.println("INFO: Turn PWM light off");
	}
}

// function called to adapt the state of the led
void setSimpleLightState() {
	if (m_simple_light_state) {
		digitalWrite(SIMPLE_LIGHT_PIN, HIGH);
		Serial.println("INFO: Simple pin on");
	} else {
		digitalWrite(SIMPLE_LIGHT_PIN, LOW);
		Serial.println("INFO: Simple light off");
	}
}

void pwmDimmTo(uint16_t dimm_to){
	// target value:  dimm_to 
	// current value: m_light_brightness
	m_pwm_dimm_target = dimm_to;

	if(!m_pwm_light_state){
		m_pwm_light_state = true;
	}
	m_pwm_dimm_state = DIMM_DIMMING; // enabled dimming
	if(millis()-m_pwm_dimm_time > 0){
		timer_dimmer = millis()-m_pwm_dimm_time;
	} else {
		timer_dimmer = 0;
	}
}

#if DHT_DS_MODE == DS
float getTemp(){ // https://blog.silvertech.at/arduino-temperatur-messen-mit-1wire-sensor-ds18s20ds18b20ds1820/
	//returns the temperature from one DS18S20 in DEG Celsius

	byte data[12];
	byte addr[8];

	if ( !ds.search(addr)) {
		//no more sensors on chain, reset search
		ds.reset_search();
		return -1000;
	}

	if ( OneWire::crc8( addr, 7) != addr[7]) {
		Serial.println("CRC is not valid!");
		return -1000;
	}

	if ( addr[0] != 0x10 && addr[0] != 0x28) {
		Serial.print("Device is not recognized");
		return -1000;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44,1); // start conversion, with parasite power on at the end

	byte present = ds.reset();
	ds.select(addr);    
	ds.write(0xBE); // Read Scratchpad


	for (int i = 0; i < 9; i++) { // we need 9 bytes
		data[i] = ds.read();
	}

	ds.reset_search();

	byte MSB = data[1];
	byte LSB = data[0];

	float tempRead = ((MSB << 8) | LSB); //using two's compliment
	float TemperatureSum = tempRead / 16;

	return TemperatureSum;  
}
#else 
float getTemp(){
	return dht.readTemperature();
}

float getHumidity(){
	return dht.readHumidity();
}
#endif

void updatePIRstate(){
	m_pir_state = digitalRead(PIR_PIN);
}

// external button push
void updateBUTTONstate(){
	// toggle, write to pin, publish to server
	m_simple_light_state = !m_simple_light_state;
	setSimpleLightState();
}

////////////////////// peripheral function ///////////////////////////////////

////////////////////// network function ///////////////////////////////////
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("\nINFO: connected");
      
      // ... and resubscribe
      client.subscribe(MQTT_PWM_LIGHT_COMMAND_TOPIC);
      client.subscribe(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC);
      client.subscribe(MQTT_SIMPLE_LIGHT_COMMAND_TOPIC);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
////////////////////// network function ///////////////////////////////////
////////////////////// SETUP ///////////////////////////////////
void setup() {
	// init the serial
	Serial.begin(115200);
  Serial.println("Boote ich nun oder nicht?");
	// init the led
	pinMode(PWM_LIGHT_PIN, OUTPUT);
	analogWriteRange(255);
	setPWMLightState();

	pinMode(SIMPLE_LIGHT_PIN, OUTPUT);
	setSimpleLightState();
	

	// init the WiFi connection
	Serial.println();
	Serial.println();
	Serial.print("INFO: Connecting to ");
	Serial.println(WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

#if DHT_DS_MODE == DHT 
	// dht
  dht.begin();
#endif

	Serial.println("");
	Serial.println("INFO: WiFi connected");
	Serial.print("INFO: IP address: ");
	Serial.println(WiFi.localIP());

	// init the MQTT connection
	client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
	client.setCallback(callback);

	// attache interrupt code for PIR
	pinMode(PIR_PIN, INPUT);
	digitalWrite(PIR_PIN,HIGH); // pull up to avoid interrupts without sensor
	attachInterrupt(digitalPinToInterrupt(PIR_PIN), updatePIRstate, CHANGE);

	// attache interrupt code for button
	pinMode(BUTTON_INPUT_PIN, INPUT);
	digitalWrite(BUTTON_INPUT_PIN,HIGH); // pull up to avoid interrupts without sensor
  attachInterrupt(digitalPinToInterrupt(BUTTON_INPUT_PIN), updateBUTTONstate, FALLING);

}
////////////////////// SETUP ///////////////////////////////////
////////////////////// LOOP ///////////////////////////////////
void loop() {
	if (!client.connected()) {
		reconnect();
	}
	client.loop();
  

	//// send periodic updates of temperature ////
	if(millis()-timer>UPDATE_TEMP){
		timer=millis();
		// request temp here
		publishTemperature(getTemp());
#if DHT_DS_MODE == DHT 
		publishHumidity(getHumidity());
#endif
    publishRssi(WiFi.RSSI());
	}
  //// send periodic updates of temperature ////

	//// dimming active?  ////
	if(m_pwm_dimm_state == DIMM_DIMMING){
		// check timestep (255ms to .. forever)
		if(timer_dimmer+m_pwm_dimm_time >= millis()){
			m_pwm_dimm_time=millis(); // save for next round

			// set new value
			if(m_light_brightness < m_pwm_dimm_target){ 
				m_light_brightness++;
			} else {
				m_light_brightness--;
			}

			// stop once you reach target
			if(m_light_brightness == m_pwm_dimm_target){
				m_pwm_dimm_state = DIMM_DONE;
			}

			// set and publish
			setPWMLightState();
		}
	}
  //// dimming end ////

  //// publish all state - ONLY after being connected for sure ////
  if(m_pir_state != m_published_pir_state){
    publishPirState();
  }

  if(m_simple_light_state != m_published_simple_light_state){
    publishSimpleLightState();
  }

  if(m_pwm_light_state != m_published_pwm_light_state){
    publishPWMLightState();
  }

  if(m_published_light_brightness != m_light_brightness){
    publishPWMLightBrightness();
  }
  //// publish all state - ONLY after being connected for sure ////
}
////////////////////// LOOP ///////////////////////////////////
