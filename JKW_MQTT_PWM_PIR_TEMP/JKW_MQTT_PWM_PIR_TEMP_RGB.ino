/*
	Highly based on a combination of different version of
	https://github.com/mertenats/Open-Home-Automation/tree/master/ha_mqtt_binary_sensor_pir
	Also using https://github.com/tzapu/WiFiManager

	Configuration (HA) :
	  light:
  - platform: mqtt ### tower living
    name: "dev1"
    state_topic: "dev1/PWM_light/status"
    command_topic: "dev1/PWM_dimm/switch"
    brightness_state_topic: 'dev1/PWM_light/brightness'
#    brightness_command_topic: 'dev1/PWM_light/brightness/set'
    brightness_command_topic: 'dev1/PWM_dimm/brightness/set'
    qos: 0
    payload_on: "ON"
    payload_off: "OFF"
    optimistic: false
    brightness_scale: 99

  config for sonoff modules:
  generic 8266
  DIO flash mode
  flash 40 mhz, cpu 80mhz 
  flash size 1mb, 64k filesystem

  This sketch (2017/01/21): 333k (Basic wifi sketch 230k)

  requires arduino libs:
  - adafruit unified sensor
  - adafruit dht22
  - onewire
*/

#include <ESP8266WiFi.h>
#include "WiFiManager.h" // local modified version          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include <OneWire.h>
#include <EEPROM.h>
#include <DHT.h>
#include <Wire.h>

//////////////////////////////////////////////////// defines //////////////////////////////////////////////
#define SERIAL_DEBUG
#define DIMM_DONE 	        0 // state defs
#define DIMM_DIMMING 	      1 // state defs
#define DHT_def             1
#define DS_def              2
#define CONFIG_SSID         "OPEN_ESP_CONFIG_AP" // SSID of the configuration mode
#define MAX_AP_TIME         300 // restart eps after 300 sec in config mode
#define TEMP_MAX            50 // DS18B20 repoorts 85.0 on first reading ... for whatever reason
// pins
#define PINOUT_SONOFF 1     // set this to "#define" for the sonoff
//#define PINOUT_KOLJA_V2         // set this to "#define" for the pcb firmware .. incosistent pinout
// D8 war nicht so gut ... startet nicht mehr 
#ifdef PINOUT_SONOFF
	#define SIMPLE_LIGHT_PIN  12 // D6
	#define DS_PIN            13 // D7
  #define PIR_PIN           14 // D5
#endif 
#ifdef PINOUT_KOLJA_V2
	#define SIMPLE_LIGHT_PIN  13 // D7
	#define DS_PIN            12 // D6
  #define PIR_PIN            5 // D1
#endif 
#define PWM_LIGHT_PIN       4  // D2

#define BUTTON_INPUT_PIN    0  // D3
#define DHT_PIN             2  // D4
#define GPIO_D8             15 // D8


#define BUTTON_TIMEOUT      1500 // max 1500ms timeout between each button press to count up (start of config)
#define BUTTON_DEBOUNCE     200 // ms debouncing for the botton
#define MSG_BUFFER_SIZE     60 // mqtt messages max char size

#define TOTAL_PERIODIC_SLOTS  5 // 5 sensors: adc, rssi, dht_hum, dht_temp, ds18b20
#define UPDATE_PERIODIC       60000/TOTAL_PERIODIC_SLOTS // update temperature once per minute .. but with 5 sensors we have to be faster
#define PUBLISH_TIME_OFFSET   200 // ms timeout between two publishes
#define UPDATE_PIR            900000L // ms timeout between two publishes of the pir .. needed?

// Buffer to hold data from the WiFi manager for mqtt login
struct mqtt_data { //80 byte
  char login[16];
  char pw[16];
  char dev_short[6];
  char server_cli_id[20];
  char server_ip[16];
  char server_port[6];
};

struct led {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
//////////////////////////////////////////////////// globals //////////////////////////////////////////////

DHT dht(DHT_PIN, DHT22); // DHT22
OneWire ds(DS_PIN);  // on digital pin DHT_DS_PIN

char char_buffer[35];

// MQTT: topics, constants, etc
const PROGMEM char*     MQTT_PWM_LIGHT_STATE_TOPIC              = "/PWM_light/status";		      // publish state here
const PROGMEM char*     MQTT_PWM_LIGHT_COMMAND_TOPIC	  	      = "/PWM_light/switch"; 		      // get command here

const PROGMEM char*     MQTT_SIMPLE_LIGHT_STATE_TOPIC		        = "/simple_light/status";	      // publish state here
const PROGMEM char*     MQTT_SIMPLE_LIGHT_COMMAND_TOPIC		      = "/simple_light/switch"; 	    // get command here

const PROGMEM char* 	  MQTT_MOTION_STATUS_TOPIC		            = "/motion/status";			        // publish
const PROGMEM char* 	  MQTT_TEMPARATURE_DS_TOPIC               = "/temperature_DS";	          // publish
const PROGMEM char*     MQTT_TEMPARATURE_DHT_TOPIC              = "/temperature_DHT";           // publish
const PROGMEM char* 	  MQTT_HUMIDITY_DHT_TOPIC			            = "/humidity_DHT";	            // publish

const PROGMEM char*     MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC   = "/PWM_light/brightness";		  // publish
const PROGMEM char*     MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC = "/PWM_light/brightness/set";	// set value

const PROGMEM char*     MQTT_PWM_DIMM_COMMAND_TOPIC             = "/PWM_dimm/switch";           // get ON/OFF command here
const PROGMEM char*     MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC 	    = "/PWM_dimm/delay/set";		    // set value
const PROGMEM char*     MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC 	= "/PWM_dimm/brightness/set";	  // set value

const PROGMEM char*     MQTT_RSSI_STATE_TOPIC                   = "/rssi";                      // publish
const PROGMEM char*     MQTT_ADC_STATE_TOPIC                    = "/adc";                       // publish

const PROGMEM char*     STATE_ON          		= "ON";
const PROGMEM char*     STATE_OFF         		= "OFF";

// variables used to store the state and the brightness of the light
boolean 	m_pwm_light_state 		  = false;
boolean     	m_published_pwm_light_state       = true; // to force instant publish once we are online
boolean		m_simple_light_state 		  = false;
boolean     	m_published_simple_light_state    = true; // to force instant publish once we are online
led 	    	m_light_target_brightness	= {99,99,99};
led 		m_light_start_brightness	= {99,99,99};
led 		m_light_current_brightness	= {99,99,99};
led 		m_light_backup_brightness	= {99,99,99};

int32_t     m_light_brightness_backup         = -1;
uint16_t     m_published_light_brightness      = 0; // to force instant publish once we are online

uint16_t		m_pwm_dimm_time				            = 10; // 10ms per Step, 255*0.01 = 2.5 sec
uint8_t			m_pwm_dimm_state			            = DIMM_DONE;

uint8_t 		m_pir_state 				              = LOW; // inaktiv
uint8_t 		m_published_pir_state             = HIGH; // to force instant publish once we are online

// converts from half log to linear .. the human eye is a smartass
uint8_t     intens[100] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,27,28,29,30,31,32,33,34,35,36,38,39,40,41,43,44,45,47,48,50,51,53,55,57,58,60,62,64,66,68,70,73,75,77,80,82,85,88,91,93,96,99,103,106,109,113,116,120,124,128,132,136,140,145,150,154,159,164,170,175,181,186,192,198,205,211,218,225,232,239,247,255};

// buffer used to send/receive data with MQTT, can not be done with the char_buffer, as both as need simultaniously 
char                  m_msg_buffer[MSG_BUFFER_SIZE];
WiFiClient            wifiClient;
WiFiManager 		      wifiManager;
PubSubClient          client(wifiClient);
mqtt_data             mqtt;

// prepare wifimanager variables
WiFiManagerParameter  WiFiManager_mqtt_server_ip("mq_ip", "mqtt server ip", "", 15);
WiFiManagerParameter  WiFiManager_mqtt_server_port("mq_port", "mqtt server port", "1883", 5);
WiFiManagerParameter  WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 6);
WiFiManagerParameter  WiFiManager_mqtt_client_id("cli_id", "mqtt client id", "18 char long desc.", 19);
WiFiManagerParameter  WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter  WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);


uint32_t 	updateFastValuesTimer	= 0;
uint32_t        updateSlowValuesTimer   = 0;
uint32_t 	timer_dimmer	= 0;
uint32_t 	timer_dimmer_start	= 0;
uint32_t 	timer_dimmer_end	= 0;
uint32_t        timer_button_down  = 0;
uint8_t         counter_button = 0;
uint32_t        timer_last_publish = 0;
uint8_t         periodic_slot = 0;

///////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// PUBLISHER ///////////////////////////////////////
// function called to publish the state of the led (on/off)
boolean publishPWMLightState() {
  boolean ret=false;
  Serial.print("[mqtt] publish PWM state ");
  if (m_pwm_light_state) {
    Serial.println("ON");
    ret = client.publish(build_topic(MQTT_PWM_LIGHT_STATE_TOPIC), STATE_ON, true);
  } else {
    Serial.println("OFF");
    ret = client.publish(build_topic(MQTT_PWM_LIGHT_STATE_TOPIC), STATE_OFF, true);
  }
  if(ret){
    m_published_pwm_light_state = m_pwm_light_state;
  }
  return ret;
}

// function called to publish the brightness of the led
boolean publishPWMLightBrightness() {
  boolean ret=false;
  Serial.print("[mqtt] publish PWM brightness ");
  Serial.println(m_light_brightness);
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_light_brightness);
  ret = client.publish(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC), m_msg_buffer, true);
  if(ret){
    m_published_light_brightness = m_light_brightness;
  }
  return ret;
}

// function called to publish the state of the led (on/off)
boolean publishSimpleLightState() {
  boolean ret=false;
  Serial.print("[mqtt] publish simple light state ");
  if (m_simple_light_state) {
    Serial.println("ON");
    ret = client.publish(build_topic(MQTT_SIMPLE_LIGHT_STATE_TOPIC), STATE_ON, true);
  } else {
    Serial.println("OFF");
    ret = client.publish(build_topic(MQTT_SIMPLE_LIGHT_STATE_TOPIC), STATE_OFF, true);
  }
  if(ret){
    m_published_simple_light_state = m_simple_light_state;
  };
  return ret;
}

// function called to publish the state of the PIR (on/off)
boolean publishPirState() {
  boolean ret=false;
  Serial.print("[mqtt] publish pir state ");
  if (m_pir_state) {
    Serial.println("motion");
    ret = client.publish(build_topic(MQTT_MOTION_STATUS_TOPIC), STATE_ON, true);
  } else {
    Serial.println("no motion");
    ret = client.publish(build_topic(MQTT_MOTION_STATUS_TOPIC), STATE_OFF, true);
  }
  if(ret){
    m_published_pir_state = m_pir_state;
  } 
  return ret;
}

// function called to publish the brightness of the led
boolean publishTemperature(float temp,int DHT_DS) {  
  if(temp>TEMP_MAX || temp<(-1*TEMP_MAX) || isnan(temp)){
    Serial.print("[mqtt] no publish temp, ");
    if(isnan(temp)){
      Serial.println("nan");  
    } else {
      Serial.print(temp);
      Serial.println(" >TEMP_MAX");
    }
    return false;
  }

  Serial.print("[mqtt] publish ");
  dtostrf(temp, 3, 2, m_msg_buffer);
  if(DHT_DS == DHT_def){
    Serial.print("DHT temp ");
    Serial.println(m_msg_buffer);
    return client.publish(build_topic(MQTT_TEMPARATURE_DHT_TOPIC), m_msg_buffer, true);
  } else {
    Serial.print("DS temp ");
    Serial.println(m_msg_buffer);
    return client.publish(build_topic(MQTT_TEMPARATURE_DS_TOPIC), m_msg_buffer, true);
  }
}

// function called to publish the brightness of the led
boolean publishHumidity(float hum) {
  if(isnan(hum)){
    Serial.println("[mqtt] no publish humidiy");
    return false;
  }
  Serial.print("[mqtt] publish humidiy ");
  dtostrf(hum, 3, 2, m_msg_buffer);
  Serial.println(m_msg_buffer);
  return client.publish(build_topic(MQTT_HUMIDITY_DHT_TOPIC), m_msg_buffer, true);
}

boolean publishRssi(float rssi) {
  Serial.print("[mqtt] publish rssi ");
  dtostrf(rssi, 3, 2, m_msg_buffer);
  Serial.println(m_msg_buffer);
  return client.publish(build_topic(MQTT_RSSI_STATE_TOPIC), m_msg_buffer, true);
}


boolean publishADC(int adc) {
  Serial.print("[mqtt] publish adc ");
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", adc);
  Serial.println(m_msg_buffer);
  return client.publish(build_topic(MQTT_ADC_STATE_TOPIC), m_msg_buffer, true);
}
//////////////////////////////////// PUBLISHER ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
///////////////////// function called when a MQTT message arrived /////////////////////
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  // handle message topic
  ////////////////////////// SET LIGHT ON/OFF ////////////////////////
  // direct set PWM value
  if (String(build_topic(MQTT_PWM_LIGHT_COMMAND_TOPIC)).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(STATE_ON))) {
      if (m_pwm_light_state != true) {
        m_pwm_light_state = true;

        // bei diesem topic hartes einschalten
        setPWMLightState();
        // Home Assistant will assume that the pwm light is 100%, once we "turn it on"
        // but it should return to whatever the m_light_brithness is, so lets set the published
        // version to something != the actual brightness. This will trigger the publishing
        m_published_light_brightness = m_light_brightness + 1;
      } else {
        // was already on .. and the received didn't know it .. so we have to re-publish
        m_published_pwm_light_state = !m_pwm_light_state;
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_pwm_light_state != false) {
        m_pwm_light_state = false;
        setPWMLightState();
      } else {
        // was already off .. and the received didn't know it .. so we have to re-publish
        m_published_pwm_light_state = !m_pwm_light_state;
      }
    }
  }
  // dimm to given PWM value
  else if (String(build_topic(MQTT_PWM_DIMM_COMMAND_TOPIC)).equals(p_topic)) {
    Serial.println("[mqtt] received dimm command");
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(STATE_ON))) {
      if (m_pwm_light_state != true) {
        //Serial.println("light was off");
        m_pwm_light_state = true;

        // bei diesem topic ein dimmen
        uint8_t keeper;
        if(m_light_brightness_backup != -1){ // we are still "dimming down", while this new command "dimm up" came in
          keeper = m_light_brightness_backup; // keep the origial value (from where we started to "dimm down")
          m_light_brightness = m_light_brightness_backup; // restore for the publish process
        } else {
          keeper = m_light_brightness; // not dimming, just remember the "old" brightness value 
        }
        //Serial.print("dimming towards ");
        //Serial.println(keeper);
        
        publishPWMLightBrightness(); // communicate where we are dimming towards
        m_light_brightness = 0; // set to zero, Dimm to will grab this as start value
        pwmDimmTo(keeper); // start dimmer towards old and therefore target value 
      } else {
        // was already on .. and the received didn't know it .. so we have to re-publish
        m_published_pwm_light_state = !m_pwm_light_state;
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_pwm_light_state != false) {
        //Serial.println("light was on");
        m_pwm_light_state = false;
        // TODO .. wir koennen wir hier ausdimmen ohne eben den m_light_brightness auf 0 zu setzen
        // a bit hacky .. we keep an backup of the old brightness, dimm to 0 and restore the backup dimm value
        m_light_brightness_backup = m_light_brightness;        
        pwmDimmTo(0); // start dimmer towards old and therefore target value 
      } else {   
        // was already off .. and the received didn't know it .. so we have to re-publish
        m_published_pwm_light_state = !m_pwm_light_state;
      }
    }
  }
  // simply switch GPIO ON/OFF
  else if (String(build_topic(MQTT_SIMPLE_LIGHT_COMMAND_TOPIC)).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(STATE_ON))) {
      if (m_simple_light_state != true) {
        m_simple_light_state = true;
        setSimpleLightState();
      } else {
        // was already on .. and the received didn't know it .. so we have to re-publish
        m_published_simple_light_state = !m_simple_light_state;
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_simple_light_state != false) {
        m_simple_light_state = false;
        setSimpleLightState();
      } else {
        // was already off .. and the received didn't know it .. so we have to re-publish
        m_published_simple_light_state = !m_simple_light_state;
      }
    }
  }
  ////////////////////////// SET LIGHT ON/OFF ////////////////////////
  ////////////////////////// SET LIGHT BRIGHTNESS ////////////////////////
  else if (String(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC)).equals(p_topic)) {
    m_light_brightness = payload.toInt();
    setPWMLightState();
  }
  else if (String(build_topic(MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC)).equals(p_topic)) {
    Serial.println("dimm input");
    pwmDimmTo(payload.toInt());
  }
  else if (String(build_topic(MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC)).equals(p_topic)) {
    m_pwm_dimm_time = payload.toInt();
    Serial.print("Setting dimm time to: ");
    Serial.println(m_pwm_dimm_time);
  }
  ////////////////////////// SET LIGHT BRIGHTNESS ////////////////////////
}
///////////////////// function called when a MQTT message arrived /////////////////////
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
////////////////////// peripheral function ///////////////////////////////////
// function called to adapt the brightness and the state of the led
void setPWMLightState() {
  setPWMLightState(false);
}
void setPWMLightState(boolean over_ride) {
  if (m_pwm_light_state || over_ride) { // allow dimming even when lights are off
    // limit max
    if(m_light_brightness>=sizeof(intens) || m_light_brightness<0){
      m_light_brightness=sizeof(intens)-1;
    }
    Serial.print("[INFO PWM] PWM Brightness: ");
    
    if (m_pwm_dimm_state == DIMM_DIMMING) {
      analogWrite(PWM_LIGHT_PIN, intens[m_pwm_dimm_current]);
      Serial.println(m_pwm_dimm_current);
    } else {
      analogWrite(PWM_LIGHT_PIN, intens[m_light_brightness]);
      Serial.println(m_light_brightness);
    };    
  } else {
    analogWrite(PWM_LIGHT_PIN, 0);
    Serial.println("[INFO PWM] Turn PWM light off");
  }
}

// function called to adapt the state of the led
void setSimpleLightState() {
  if (m_simple_light_state) {
    digitalWrite(SIMPLE_LIGHT_PIN, HIGH);
    Serial.println("[INFO SL] Simple pin on");
  } else {
    digitalWrite(SIMPLE_LIGHT_PIN, LOW);
    Serial.println("[INFO SL] Simple light off");
  }
}

void pwmDimmTo(led dimm_to) {
	// find biggest difference for end time calucation
	uint8_t biggest_delta=0
	uint8_t d=abs(m_light_current_brightness.r-dimm_to.r);
	if(d>biggest_delta){
		biggest_delta=d;
	}
	d=abs(m_light_current_brightness.g-dimm_to.g);
	if(d>biggest_delta){
		biggest_delta=d;
	}
	d=abs(m_light_current_brightness.b-dimm_to.b);
	if(d>biggest_delta){
		biggest_delta=d;
	}		
	m_light_start_brightness = m_light_current_brightness;

	timer_dimmer_start = millis()
	timer_dimmer_end = timer_dimmer_start+biggest_delta * m_pwm_dimm_time;

	//Serial.print("Enabled dimming, timing: ");
	//Serial.println(m_pwm_dimm_time);
}

float getDsTemp() { // https://blog.silvertech.at/arduino-temperatur-messen-mit-1wire-sensor-ds18s20ds18b20ds1820/
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -999;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -888;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -777;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

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

float getDhtTemp() {
  return dht.readTemperature();
}

float getDhtHumidity() {
  return dht.readHumidity();
}

void updatePIRstate() {
  m_pir_state = digitalRead(PIR_PIN);
}

// external button push
void updateBUTTONstate() {
  // toggle, write to pin, publish to server
  if(digitalRead(BUTTON_INPUT_PIN)==LOW){
    if(millis()-timer_button_down>BUTTON_DEBOUNCE){ // avoid bouncing
      // button down
      // toggle status of both lights
      m_simple_light_state = !m_simple_light_state;
      setSimpleLightState();

      // Home Assistant will assume that the pwm light is 100%, once we report toggle to on
      // but it should return to whatever the m_light_brithness is, so lets set the published
      // version to something != the actual brightness. This will trigger the publishing
      m_pwm_light_state = !m_pwm_light_state;
      setPWMLightState();
      m_published_light_brightness = m_light_brightness + 1;
      // toggle status of both lights

      if(millis()-timer_button_down<BUTTON_TIMEOUT){
        counter_button++;
      } else {
        counter_button=1;
      }
      Serial.print("[BUTTON] push nr ");
      Serial.println(counter_button);

    };
    //Serial.println("DOWN");
    timer_button_down = millis();
  };
}

/////////////////////////////// peripheral function ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// network function ///////////////////////////////////
void reconnect() {
  // Loop until we're reconnected
  uint8_t tries=0;
  while (!client.connected()) {
    Serial.println("[mqtt] Attempting connection...");
    // Attempt to connect
    Serial.print("[mqtt] client id: ");
    Serial.println(mqtt.server_cli_id);
    if (client.connect(mqtt.server_cli_id, mqtt.login, mqtt.pw)) {
      Serial.println("[mqtt] connected");

      // ... and resubscribe
      client.subscribe(build_topic(MQTT_PWM_LIGHT_COMMAND_TOPIC));  // hard on off
      client.subscribe(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC));  // direct bright
      
      client.subscribe(build_topic(MQTT_SIMPLE_LIGHT_COMMAND_TOPIC)); // on off
      
      client.subscribe(build_topic(MQTT_PWM_DIMM_COMMAND_TOPIC)); // dimm on 
      client.subscribe(build_topic(MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC));
      client.subscribe(build_topic(MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC));
      Serial.println("[mqtt] subscribed");

    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println(", DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    tries++;
    if(tries>=5){
      Serial.println("Can't connect, starting AP");
      wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
    }
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  wifiManager.addParameter(&WiFiManager_mqtt_server_ip);
  wifiManager.addParameter(&WiFiManager_mqtt_server_port);
  wifiManager.addParameter(&WiFiManager_mqtt_client_id);
  wifiManager.addParameter(&WiFiManager_mqtt_client_short);
  wifiManager.addParameter(&WiFiManager_mqtt_server_login);
  wifiManager.addParameter(&WiFiManager_mqtt_server_pw);
  // prepare wifimanager variables
  wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,255), IPAddress(255,255,255,0));
  Serial.println("Entered config mode");
}

// save config to eeprom
void saveConfigCallback(){ 
  sprintf(mqtt.server_ip, "%s", WiFiManager_mqtt_server_ip.getValue());
  sprintf(mqtt.login, "%s", WiFiManager_mqtt_server_login.getValue());
  sprintf(mqtt.pw, "%s", WiFiManager_mqtt_server_pw.getValue());
  sprintf(mqtt.server_port, "%s", WiFiManager_mqtt_server_port.getValue());
  sprintf(mqtt.server_cli_id, "%s", WiFiManager_mqtt_client_id.getValue());
  sprintf(mqtt.dev_short, "%s", WiFiManager_mqtt_client_short.getValue());
  Serial.println(("=== Saving parameters: ==="));
  Serial.print(("mqtt ip: ")); Serial.println(mqtt.server_ip);
  Serial.print(("mqtt port: ")); Serial.println(mqtt.server_port);
  Serial.print(("mqtt user: ")); Serial.println(mqtt.login);
  Serial.print(("mqtt pw: ")); Serial.println(mqtt.pw);
  Serial.print(("mqtt client id: ")); Serial.println(mqtt.server_cli_id);
  Serial.print(("mqtt dev short: ")); Serial.println(mqtt.dev_short);
  Serial.println(("=== End of parameters ===")); 
  char* temp=(char*) &mqtt;
  for(int i=0; i<sizeof(mqtt); i++){
    EEPROM.write(i,*temp);
    //Serial.print(*temp);
    temp++;
  }
  EEPROM.commit();
  Serial.println("Configuration saved, restarting");
  delay(2000);  
  ESP.reset(); // eigentlich muss das gehen so, .. // we can't change from AP mode to client mode, thus: reboot
}

void loadConfig(){
  // fill the mqtt element with all the data from eeprom
  char* temp=(char*) &mqtt;
  for(int i=0; i<sizeof(mqtt); i++){
    //Serial.print(i);
    *temp = EEPROM.read(i);
    //Serial.print(*temp);
    temp++;
  }
  Serial.println(("=== Loaded parameters: ==="));
  Serial.print(("mqtt ip: "));        Serial.println(mqtt.server_ip);
  Serial.print(("mqtt port: "));      Serial.println(mqtt.server_port);
  Serial.print(("mqtt user: "));      Serial.println(mqtt.login);
  Serial.print(("mqtt pw: "));        Serial.println(mqtt.pw);
  Serial.print(("mqtt client id: ")); Serial.println(mqtt.server_cli_id);
  Serial.print(("mqtt dev short: ")); Serial.println(mqtt.dev_short);
  Serial.println(("=== End of parameters ==="));
}

// build topics with device id on the fly
char* build_topic(const char *topic) {
  sprintf(char_buffer, "%s%s", mqtt.dev_short, topic);
  return char_buffer;
}
////////////////////////////////// network function ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////// SETUP ///////////////////////////////////
void setup() {
  // init the serial
  Serial.begin(115200);
  Serial.println("");
  Serial.print("Startup ");
  Serial.println("v2.411");
  EEPROM.begin(512); // can be up to 4096
 
  // init the led
  pinMode(PWM_LIGHT_PIN, OUTPUT);
  analogWriteRange(255);
  setPWMLightState();

  pinMode(SIMPLE_LIGHT_PIN, OUTPUT);
  setSimpleLightState();

  pinMode(GPIO_D8,OUTPUT);

  // attache interrupt code for PIR
  pinMode(PIR_PIN, INPUT);
  digitalWrite(PIR_PIN, HIGH); // pull up to avoid interrupts without sensor
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), updatePIRstate, CHANGE);

  // attache interrupt code for button
  pinMode(BUTTON_INPUT_PIN, INPUT);
  digitalWrite(BUTTON_INPUT_PIN, HIGH); // pull up to avoid interrupts without sensor
  attachInterrupt(digitalPinToInterrupt(BUTTON_INPUT_PIN), updateBUTTONstate, CHANGE);
  
  // start wifi manager
  Serial.println();
  Serial.println();
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(MAX_AP_TIME);
  
  if(digitalRead(BUTTON_INPUT_PIN)==LOW){
    wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
  };

  Serial.println("[WiFi] Connecting ");
  if(!wifiManager.autoConnect(CONFIG_SSID)){
    // possible situataion: Main power out, ESP went to config mode as the routers wifi wasn available on time .. 
    Serial.println("failed to connect and hit timeout, restarting ..");
    delay(1000); // time for serial to print
    ESP.reset(); // reset loop if not only or configured after 5min .. 
  }

  // load all paramters!
  loadConfig();
  
   // dht
  dht.begin();

  Serial.println("");
  Serial.println("[WiFi] connected");
  Serial.print("[WiFi] IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(mqtt.server_ip, atoi(mqtt.server_port));
  client.setCallback(callback);
}
///////////////////////////////////////////// SETUP ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// LOOP ///////////////////////////////////
void loop() {
	if (!client.connected()) {
	  reconnect();
	}
	client.loop();
  
	//// dimming active?  ////
	if(millis() <= timer_dimmer_end){
		if(millis() >= timer_dimmer + m_pwm_dimm_time) {
			//Serial.print("DIMMER ");
			timer_dimmer = millis(); // save for next round

			// set new value
			m_light_current_brightness.r = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start_brightness.r, m_light_target_brightness.r);
			m_light_current_brightness.g = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start_brightness.g, m_light_target_brightness.g);
			m_light_current_brightness.b = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start_brightness.b, m_light_target_brightness.b);
      
			//Serial.println(m_light_brightness);
			setPWMLightState(true); // with override 

		}
	}
	//// dimming end ////

	//// publish all state - ONLY after being connected for sure ////

	if (m_simple_light_state != m_published_simple_light_state) {
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishSimpleLightState();
			timer_last_publish = millis();
		}
	}

	if (m_pwm_light_state != m_published_pwm_light_state) {
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishPWMLightState();
			timer_last_publish = millis();
		};
	}

	if (m_published_light_brightness != m_light_brightness) { // todo .. fails to publish if we publish to much at the first run ... 
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishPWMLightBrightness();
			timer_last_publish = millis();
		}
	}

	if (m_pir_state != m_published_pir_state) {
		if(millis()-timer_last_publish >PUBLISH_TIME_OFFSET){
			publishPirState();  
			timer_last_publish = millis();
		}
	}
	//// publish all state - ONLY after being connected for sure ////


	//// send periodic updates ////
	if ( millis() - updateFastValuesTimer > UPDATE_PERIODIC && millis()-timer_last_publish > PUBLISH_TIME_OFFSET) {
		updateFastValuesTimer = millis();

		if(periodic_slot == 0){
			publishTemperature(getDhtTemp(),DHT_def);
		} else if(periodic_slot == 1){
			publishTemperature(getDsTemp(),DS_def);
		} else if(periodic_slot == 2){
			publishHumidity(getDhtHumidity());
		} else if(periodic_slot == 3){
			publishRssi(WiFi.RSSI());
		} else if(periodic_slot == 4){
			digitalWrite(GPIO_D8,HIGH);
			delay(100);
			publishADC(analogRead(A0));
			digitalWrite(GPIO_D8,LOW);
		};
		periodic_slot = (periodic_slot+1)%TOTAL_PERIODIC_SLOTS;
	}
	//// send periodic updates  ////
	//// send periodic updates of PIR... just in case  ////
	if ( millis() - updateSlowValuesTimer > UPDATE_PIR && millis()-timer_last_publish > PUBLISH_TIME_OFFSET) {
		updateSlowValuesTimer = millis();
		timer_last_publish = millis();
		publishPirState();  
	}
	//// send periodic updates of PIR ... just in case ////



	/// see if we hold down the button for more then 6sec /// 
	if(counter_button>=10 && millis()-timer_button_down>BUTTON_TIMEOUT){
		Serial.println("[SYS] Rebooting to setup mode");
		delay(200);
		wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
		//ESP.reset(); // reboot and switch to setup mode right after that
	}
	/// see if we hold down the button for more then 6sec /// 
}
////////////////////////////////////////////// LOOP ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
