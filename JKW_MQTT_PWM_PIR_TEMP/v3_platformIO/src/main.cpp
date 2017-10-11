#include "main.h"
//////////////////////////////////////////////////// globals //////////////////////////////////////////////

DHT dht(DHT_PIN, DHT22); // DHT22
OneWire ds(DS_PIN);  // on digital pin DHT_DS_PIN
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NEOPIXEL_LED_COUNT, PWM_LIGHT_PIN2); // this version only works on gpio3 / D9 (RX)

char char_buffer[35];

// MQTT: topics, constants, etc
const PROGMEM char* MQTT_PWM_LIGHT_STATE_TOPIC                = "/PWM_light/status";		      // publish state here ON / OFF
const PROGMEM char* MQTT_PWM_LIGHT_COMMAND_TOPIC	            = "/PWM_light/switch"; 		      // get command here ON / OFF

const PROGMEM char* MQTT_SIMPLE_LIGHT_STATE_TOPIC		          = "/simple_light/status";	      // publish state here ON / OFF
const PROGMEM char* MQTT_SIMPLE_LIGHT_COMMAND_TOPIC		        = "/simple_light/switch"; 	    // get command here ON / OFF

const PROGMEM char* MQTT_RAINBOW_STATUS_TOPIC                 = "/rainbow/status";            // publish state here ON / OFF
const PROGMEM char* MQTT_RAINBOW_COMMAND_TOPIC                = "/rainbow/switch";            // get command here ON / OFF
const PROGMEM char* MQTT_SIMPLE_RAINBOW_STATUS_TOPIC          = "/simple_rainbow/status";     // publish state here ON / OFF
const PROGMEM char* MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC         = "/simple_rainbow/switch";     // get command here ON / OFF
const PROGMEM char* MQTT_COLOR_WIPE_STATUS_TOPIC              = "/color_wipe/status";         // publish state here ON / OFF
const PROGMEM char* MQTT_COLOR_WIPE_COMMAND_TOPIC             = "/color_wipe/switch";         // get command here ON / OFF

const PROGMEM char* MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC     = "/PWM_light/brightness";		  // publish 0-99
const PROGMEM char* MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC   = "/PWM_light/brightness/set";	// set value 0-99
const PROGMEM char* MQTT_PWM_DIMM_COMMAND_TOPIC               = "/PWM_dimm/switch";           // get ON/OFF command here

const PROGMEM char* MQTT_PWM_RGB_DIMM_COLOR_STATE_TOPIC       = "/PWM_RGB_light/color";		    // publish value "0-99,0-99,0-99"
const PROGMEM char* MQTT_PWM_RGB_COLOR_COMMAND_TOPIC          = "/PWM_RGB_light/color/set";		// set value "0-99,0-99,0-99"
const PROGMEM char* MQTT_PWM_RGB_DIMM_COLOR_COMMAND_TOPIC     = "/PWM_RGB_dimm/color/set";		// set value "0-99,0-99,0-99"

const PROGMEM char* MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC 	      = "/PWM_dimm/delay/set";		    // set value
const PROGMEM char* MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC 	  = "/PWM_dimm/brightness/set";	  // set value

const PROGMEM char* MQTT_MOTION_STATUS_TOPIC		              = "/motion/status";			        // publish
const PROGMEM char* MQTT_TEMPARATURE_DS_TOPIC                 = "/temperature_DS";	          // publish
const PROGMEM char* MQTT_TEMPARATURE_DHT_TOPIC                = "/temperature_DHT";           // publish
const PROGMEM char* MQTT_HUMIDITY_DHT_TOPIC			              = "/humidity_DHT";	            // publish
const PROGMEM char* MQTT_BUTTON_TOPIC                         = "/button";                    // publish
const PROGMEM char* MQTT_RSSI_STATE_TOPIC                     = "/rssi";                      // publish
const PROGMEM char* MQTT_ADC_STATE_TOPIC                      = "/adc";                       // publish
const PROGMEM char* MQTT_SETUP_TOPIC                        	= "/setup";                     // subscribe

const PROGMEM char* STATE_ON     	= "ON";
const PROGMEM char* STATE_OFF    	= "OFF";

// variables used to store the state and the brightness of the light
boolean		m_pwm_light_state = false;
boolean		m_published_pwm_light_state = true; // to force instant publish once we are online
boolean		m_simple_light_state = false;
boolean		m_published_simple_light_state = true; // to force instant publish once we are online
boolean   m_published_button_press = true;
boolean   m_button_press = true; // same to avoid button press send

led		m_light_target		=  {99,99,99};
led		m_light_start		  =  {99,99,99};
led		m_light_current	  =  {99,99,99}; // actual value written to PWM
led		m_light_backup	  =  {99,99,99}; // to be able to resume "dimm on" to the last set color

// converts from half log to linear .. the human eye is a smartass
uint8_t     intens[100] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,27,28,29,30,31,32,33,34,35,36,38,39,40,41,43,44,45,47,48,50,51,53,55,57,58,60,62,64,66,68,70,73,75,77,80,82,85,88,91,93,96,99,103,106,109,113,116,120,124,128,132,136,140,145,150,154,159,164,170,175,181,186,192,198,205,211,218,225,232,239,247,255};

uint8_t   m_published_light_brightness = 1; // to force instant publish once we are online
uint16_t  m_published_light_color = 1; // to force instant publish once we are online (sum of rgb)
uint16_t	m_pwm_dimm_time = 10; // 10ms per Step, 255*0.01 = 2.5 sec
boolean   m_use_neo_as_rgb = false; // if true we're going to send the color to the neopixel and not to the pwm pins

boolean   m_avoid_relay = false;
uint8_t 	m_pir_state = LOW; // inaktiv
uint8_t 	m_published_pir_state = HIGH; // to force instant publish once we are online

uint8_t   m_animation_type = 0; // type 1 = rainbow wheel, 2 = simple rainbow .. see define above
uint8_t   m_published_animation_type = -1; // on off published
uint8_t   m_animation_pos = 0; // pointer im wheel


// buffer used to send/receive data with MQTT, can not be done with the char_buffer, as both as need simultaniously
char                  m_msg_buffer[MSG_BUFFER_SIZE];
WiFiClient            wifiClient;
WiFiManager 		      wifiManager;
PubSubClient          client(wifiClient);
mqtt_data             mqtt;

// prepare wifimanager variables
WiFiManagerParameter  WiFiManager_mqtt_server_ip("mq_ip", "mqtt server ip", "", 15);
WiFiManagerParameter  WiFiManager_mqtt_server_port("mq_port", "mqtt server port", "1883", 5);
WiFiManagerParameter  WiFiManager_mqtt_capability("cap", "Capability Bit0 = PWM, Bit1 = Neopixel, Bit2 = Avoid relay", "", 2);
WiFiManagerParameter  WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 6);
WiFiManagerParameter  WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter  WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);


uint32_t 	      updateFastValuesTimer	= 0;
uint32_t        updateSlowValuesTimer   = 0;
uint32_t 	      timer_dimmer	= 0;
uint32_t 	      timer_dimmer_start	= 0;
uint32_t 	      timer_dimmer_end	= 0;
uint32_t        timer_republish_avoid = 0;
uint32_t        timer_button_down  = 0;
uint8_t         counter_button = 0;
uint32_t        timer_last_publish = 0;
uint8_t         periodic_slot = 0;
uint32_t        m_neopixel_dimm_time = 0;

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
  Serial.println(m_light_target.r);
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_light_target.r);
  ret = client.publish(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC), m_msg_buffer, true);
  if(ret){
    m_published_light_brightness = m_light_target.r;
  }
  return ret;
}

// function called to publish the color of the led
boolean publishPWMRGBColor(){
	boolean ret=false;
	Serial.print("[mqtt] publish PWM color ");
	snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_light_target.r, m_light_target.g, m_light_target.b);
	Serial.println(m_msg_buffer);
	ret = client.publish(build_topic(MQTT_PWM_RGB_DIMM_COLOR_STATE_TOPIC), m_msg_buffer, true);
	if(ret){
		m_published_light_color = m_light_target.r + m_light_target.g + m_light_target.b;
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

boolean publishButtonPush(){
  boolean ret=false;
  Serial.println("[mqtt] publish button push");
  ret = client.publish(build_topic(MQTT_BUTTON_TOPIC), "", true);
  if(ret){
    m_published_button_press = m_button_press;
  }
  return ret;
}

// function called to publish the state of the rainbow (on/off)
boolean publishAnimationType() {
  boolean ret=true;

  Serial.print("[mqtt] publish rainbow state ");
  if(m_animation_type==ANIMATION_RAINBOW_WHEEL) {
    Serial.println("on");
    ret &= client.publish(build_topic(MQTT_RAINBOW_STATUS_TOPIC), STATE_ON, true);
  } else {
    Serial.println("off");
    ret &= client.publish(build_topic(MQTT_RAINBOW_STATUS_TOPIC), STATE_OFF, true);
  }

  Serial.print("[mqtt] publish simple rainbow state ");
  if(m_animation_type==ANIMATION_RAINBOW_SIMPLE) {
    Serial.println("on");
    ret &= client.publish(build_topic(MQTT_SIMPLE_RAINBOW_STATUS_TOPIC), STATE_ON, true);
  } else {
    Serial.println("off");
    ret &= client.publish(build_topic(MQTT_SIMPLE_RAINBOW_STATUS_TOPIC), STATE_OFF, true);
  }

  Serial.print("[mqtt] publish color WIPE state ");
  if(m_animation_type==ANIMATION_COLOR_WIPE) {
    Serial.println("on");
    ret &= client.publish(build_topic(MQTT_COLOR_WIPE_STATUS_TOPIC), STATE_ON, true);
  } else {
    Serial.println("off");
    ret &= client.publish(build_topic(MQTT_COLOR_WIPE_STATUS_TOPIC), STATE_OFF, true);
  }


  if(ret){
    m_published_animation_type = m_animation_type;
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
        m_animation_type = ANIMATION_OFF; // publish that we've switched if off .. just in case

        // bei diesem topic hartes einschalten
        m_light_current = m_light_backup;
        setPWMLightState();
        // Home Assistant will assume that the pwm light is 100%, once we "turn it on"
        // but it should return to whatever the m_light_brithness is, so lets set the published
        // version to something != the actual brightness. This will trigger the publishing
        m_published_light_brightness = m_light_target.r + 1;
      } else {
        // was already on .. and the received didn't know it .. so we have to re-publish
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_pwm_light_state = !m_pwm_light_state;
        }
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_pwm_light_state != false) {
        m_pwm_light_state = false;
		    m_light_backup = m_light_target; // save last target value to resume later on
		    m_light_current = (led){0,0,0};
        setPWMLightState();
      } else {
        // was already off .. and the received didn't know it .. so we have to re-publish
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_pwm_light_state = !m_pwm_light_state;
        }
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
        pwmDimmTo(m_light_backup);
        publishPWMLightBrightness(); // communicate where we are dimming towards
		    publishPWMRGBColor(); // communicate where we are dimming towards
      } else {
        // was already on .. and the received didn't know it .. so we have to re-publish
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_pwm_light_state = !m_pwm_light_state;
        }
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_pwm_light_state != false) {
        //Serial.println("light was on");
        m_pwm_light_state = false;
        // remember the current target value to resume to this value once we receive a 'dimm on 'command
        m_light_backup = m_light_target;  // target instead off current (just in case we are currently dimming)
        pwmDimmTo((led){0,0,0}); // dimm off
        publishPWMLightBrightness(); // communicate where we are dimming towards
		    publishPWMRGBColor(); // communicate where we are dimming towards
      } else {
        // was already off .. and the received didn't know it .. so we have to re-publish
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_pwm_light_state = !m_pwm_light_state;
        }
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
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_simple_light_state = !m_simple_light_state;
        }
      }
    } else if (payload.equals(String(STATE_OFF))) {
      if (m_simple_light_state != false) {
        m_simple_light_state = false;
        setSimpleLightState();
      } else {
        // was already off .. and the received didn't know it .. so we have to re-publish
        // but only if the last pubish is less then 3 sec ago
        if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
          m_published_simple_light_state = !m_simple_light_state;
        }
      }
    }
  }
  ////////////////////////// SET LIGHT ON/OFF ////////////////////////
  ////////////////////////// SET RAINBOW /////////////////
  // simply switch GPIO ON/OFF
  else if (String(build_topic(MQTT_RAINBOW_COMMAND_TOPIC)).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    Serial.print("INPUT!");
    if (payload.equals(String(STATE_ON)) && m_animation_type != ANIMATION_RAINBOW_WHEEL) {
      setAnimationType(ANIMATION_RAINBOW_WHEEL);
    } else if (payload.equals(String(STATE_OFF)) && m_animation_type == ANIMATION_RAINBOW_WHEEL) {
      setAnimationType(ANIMATION_OFF);
    } else {
      // was already in the state .. and the received didn't know it .. so we have to re-publish
      // but only if the last pubish is less then 3 sec ago
      if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
        m_published_animation_type = ANIMATION_OFF;
      }
    }
  }
  ////////////////////////// SET RAINBOW /////////////////
  ////////////////////////// SET SIMPLE RAINBOW /////////////////
  // simply switch GPIO ON/OFF
  else if (String(build_topic(MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC)).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    Serial.print("INPUT!");
    if (payload.equals(String(STATE_ON)) && m_animation_type != ANIMATION_RAINBOW_SIMPLE) {
      setAnimationType(ANIMATION_RAINBOW_SIMPLE);
    } else if (payload.equals(String(STATE_OFF)) && m_animation_type == ANIMATION_RAINBOW_SIMPLE) {
      setAnimationType(ANIMATION_OFF);
    } else {
      // was already in the state .. and the received didn't know it .. so we have to re-publish
      // but only if the last pubish is less then 3 sec ago
      if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
        m_published_animation_type = ANIMATION_OFF;
      }
    }
  }
  ////////////////////////// SET SIMPLE RAINBOW /////////////////
  ////////////////////////// SET SIMPLE COLOR WIPE /////////////////
  // simply switch GPIO ON/OFF
  else if (String(build_topic(MQTT_COLOR_WIPE_COMMAND_TOPIC)).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    Serial.print("INPUT!");
    if (payload.equals(String(STATE_ON)) && m_animation_type != ANIMATION_COLOR_WIPE) {
      setAnimationType(ANIMATION_COLOR_WIPE);
    } else if (payload.equals(String(STATE_OFF)) && m_animation_type == ANIMATION_COLOR_WIPE) {
      setAnimationType(ANIMATION_OFF);
    } else {
      // was already in the state .. and the received didn't know it .. so we have to re-publish
      // but only if the last pubish is less then 3 sec ago
      if(timer_republish_avoid+REPUBISH_AVOID_TIMEOUT<millis()){
        m_published_animation_type = ANIMATION_OFF;
      }
    }
  }
  ////////////////////////// SET SIMPLE COLOR WIPE /////////////////
  ////////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
  else if (String(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC)).equals(p_topic)) {	// directly set the PWM, hard
    m_light_current.r = payload.toInt(); // das regelt die helligkeit
    setPWMLightState();
  }
  else if (String(build_topic(MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC)).equals(p_topic)) { // smooth dimming of pwm
    Serial.print("pwm dimm input ");
    Serial.println((uint8_t)payload.toInt());
    pwmDimmTo((led){(uint8_t)payload.toInt(),(uint8_t)0,(uint8_t)0});
  }
  else if (String(build_topic(MQTT_PWM_RGB_COLOR_COMMAND_TOPIC)).equals(p_topic)) { // directly set rgb, hard
    Serial.println("set input hard");
    uint8_t firstIndex = payload.indexOf(',');
    uint8_t lastIndex = payload.lastIndexOf(',');
    m_light_current = (led){
      (uint8_t)map((uint8_t)payload.substring(0, firstIndex).toInt(),0,255,0,99),
      (uint8_t)map((uint8_t)payload.substring(firstIndex + 1, lastIndex).toInt(),0,255,0,99),
      (uint8_t)map((uint8_t)payload.substring(lastIndex + 1).toInt(),0,255,0,99)
    };
    m_light_target = m_light_current;
    setPWMLightState();
  }
  else if (String(build_topic(MQTT_PWM_RGB_DIMM_COLOR_COMMAND_TOPIC)).equals(p_topic)) { // smoothly dimm to rgb value
    Serial.println("color dimm input");
    uint8_t firstIndex = payload.indexOf(',');
    uint8_t lastIndex = payload.lastIndexOf(',');
    //input color values are 888rgb .. we need precentage
    pwmDimmTo((led){
      (uint8_t)map((uint8_t)payload.substring(0, firstIndex).toInt(),0,255,0,99),
      (uint8_t)map((uint8_t)payload.substring(firstIndex + 1, lastIndex).toInt(),0,255,0,99),
      (uint8_t)map((uint8_t)payload.substring(lastIndex + 1).toInt(),0,255,0,99)
    });
  }
  else if (String(build_topic(MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC)).equals(p_topic)) { // adjust dimmer delay
    m_pwm_dimm_time = payload.toInt();
    //Serial.print("Setting dimm time to: ");
    //Serial.println(m_pwm_dimm_time);
  }
  else if (String(build_topic(MQTT_SETUP_TOPIC)).equals(p_topic)) {
    if (payload.equals(String(STATE_ON))) { // go to setup
      Serial.println("mqtt input: go to setup");
      delay(500);
      if(m_use_neo_as_rgb){ // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
        Serial.end();
        delay(500);
        Serial.begin(115200);
      }
      client.publish(build_topic(MQTT_SETUP_TOPIC), "ok", true);
    	wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
    }
    else if(payload.substring(0,4).equals(String("http"))){ // update
      Serial.print("Update command with url found, trying to update from ");
      Serial.println(payload);
      client.publish(build_topic(MQTT_SETUP_TOPIC), "ok", true);
    	ESPhttpUpdate.update(payload);
    }
    else if(payload.equals(String("reset"))){ // reboot
      Serial.print("Received reset command");
      ESP.reset();
    }
  }
  ////////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
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
  // make sure that we never have a brightness of zero when we're switched on
  // rather set it to full brightness then to have it auto-off again
  if(m_pwm_light_state && m_light_target.r==0){
    m_light_current.r=sizeof(intens)-1;
  }

  // limit max
  if(m_light_current.r>=sizeof(intens)){
    m_light_current.r=sizeof(intens)-1;
  }
  if(m_light_current.g>=sizeof(intens)){
    m_light_current.g=sizeof(intens)-1;
  }
  if(m_light_current.b>=sizeof(intens)){
    m_light_current.b=sizeof(intens)-1;
  }
  Serial.print("[INFO PWM] PWM: ");
  snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_light_current.r, m_light_current.g, m_light_current.b);
  Serial.println(m_msg_buffer);


  if(m_use_neo_as_rgb){
   for(int i=0; i<NEOPIXEL_LED_COUNT; i++) {
      strip.SetPixelColor(i,RgbColor(intens[m_light_current.r],intens[m_light_current.g],intens[m_light_current.b]));
    }
    strip.Show();
  } else {
    analogWrite(PWM_LIGHT_PIN1, intens[m_light_current.r]);
    analogWrite(PWM_LIGHT_PIN2, intens[m_light_current.g]);
    analogWrite(PWM_LIGHT_PIN3, intens[m_light_current.b]);
  }
}

// function called to adapt the state of the led
void setSimpleLightState() {
  if (m_simple_light_state) {
    if(!m_avoid_relay){
      digitalWrite(SIMPLE_LIGHT_PIN, HIGH);
    }
    Serial.println("[INFO SL] Simple pin on");
  } else {
    if(!m_avoid_relay){
      digitalWrite(SIMPLE_LIGHT_PIN, LOW);
    }
    Serial.println("[INFO SL] Simple light off");
  }
}

// set rainbow start point .. thats pretty much it
void setAnimationType(int type){
  m_animation_pos = 0; // start all animation from beginning
  m_neopixel_dimm_time = millis();
  m_animation_type = type;
  // switch off
  if(m_animation_type==ANIMATION_OFF){
    for(int i=0; i<NEOPIXEL_LED_COUNT; i++) {
      strip.SetPixelColor(i,RgbColor(0,0,0));
    }
    strip.Show();
  }
}

void pwmDimmTo(led dimm_to) {
  Serial.print("pwmDimmTo: ");
  Serial.print(dimm_to.r);
  Serial.print(",");
  Serial.print(dimm_to.g);
  Serial.print(",");
  Serial.println(dimm_to.b);
	// find biggest difference for end time calucation
	m_light_target = dimm_to;
	// TODO: do we have to multiply that with the brightness value?

	uint8_t biggest_delta=0;
	uint8_t d=abs(m_light_current.r-dimm_to.r);
	if(d>biggest_delta){
		biggest_delta=d;
	}
	d=abs(m_light_current.g-dimm_to.g);
	if(d>biggest_delta){
		biggest_delta=d;
	}
	d=abs(m_light_current.b-dimm_to.b);
	if(d>biggest_delta){
		biggest_delta=d;
	}
	m_light_start = m_light_current;

	timer_dimmer_start = millis();
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
      m_button_press=!m_published_button_press;
      // toggle status of both lights
      m_simple_light_state = !m_simple_light_state;
      setSimpleLightState();

      // Home Assistant will assume that the pwm light is 100%, once we report toggle to on
      // but it should return to whatever the m_light_brithness is, so lets set the published
      // version to something != the actual brightness. This will trigger the publishing
      m_pwm_light_state = !m_pwm_light_state;
      setPWMLightState();
      m_published_light_brightness = m_light_target.r + 1;
      // toggle status of both lights

      if(millis()-timer_button_down<BUTTON_TIMEOUT){
        counter_button++;
      } else {
        counter_button=1;
      }
      Serial.print("[BUTTON] push nr ");
      Serial.println(counter_button);

    };
    //Serial.print(".");
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
    Serial.println(mqtt.dev_short);
    if (client.connect(mqtt.dev_short, mqtt.login, mqtt.pw)) {
      Serial.println("[mqtt] connected");

      // ... and resubscribe
      client.subscribe(build_topic(MQTT_PWM_LIGHT_COMMAND_TOPIC));  // hard on off
      client.loop();
      Serial.println("[mqtt] subscribed 1");
      client.subscribe(build_topic(MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC));  // direct bright
      client.loop();
      Serial.println("[mqtt] subscribed 2");
      client.subscribe(build_topic(MQTT_SIMPLE_LIGHT_COMMAND_TOPIC)); // on off
      client.loop();
      Serial.println("[mqtt] subscribed 3");
      client.subscribe(build_topic(MQTT_PWM_DIMM_COMMAND_TOPIC)); // dimm on
      client.loop();
      Serial.println("[mqtt] subscribed 4");
      client.subscribe(build_topic(MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC));
      client.loop();
      Serial.println("[mqtt] subscribed 5");
      client.subscribe(build_topic(MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC));
      client.loop();
      Serial.println("[mqtt] subscribed 6");
      client.subscribe(build_topic(MQTT_PWM_RGB_DIMM_COLOR_COMMAND_TOPIC)); // color topic
      client.loop();
      Serial.println("[mqtt] subscribed 7");
      client.subscribe(build_topic(MQTT_SETUP_TOPIC)); // color topic
      client.loop();
      if(m_use_neo_as_rgb){
        Serial.println("[mqtt] subscribed 8");
        client.subscribe(build_topic(MQTT_RAINBOW_COMMAND_TOPIC)); // rainbow  topic
        client.loop();
        Serial.println("[mqtt] subscribed 9");
        client.subscribe(build_topic(MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC)); // simple rainbow  topic
        client.loop();
        Serial.println("[mqtt] subscribed 10");
        client.subscribe(build_topic(MQTT_COLOR_WIPE_COMMAND_TOPIC)); // color WIPE topic
        client.loop();
        Serial.println("[mqtt] subscribed 11");
      }
      Serial.println("[mqtt] subscribing finished");
      snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%s %s", PINOUT, VERSION);
      //Serial.println(build_topic("/INFO"));
      //Serial.println(m_msg_buffer);
      client.publish(build_topic("/INFO"), m_msg_buffer, true);
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
      if(m_use_neo_as_rgb){ // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
        Serial.end();
        delay(500);
        Serial.begin(115200);
      }
      wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
    }
  }
}

void configModeCallback(WiFiManager *myWiFiManager) {
  wifiManager.addParameter(&WiFiManager_mqtt_server_ip);
  wifiManager.addParameter(&WiFiManager_mqtt_server_port);
  wifiManager.addParameter(&WiFiManager_mqtt_capability);
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
  sprintf(mqtt.cap, "%s", WiFiManager_mqtt_capability.getValue());
  sprintf(mqtt.server_port, "%s", WiFiManager_mqtt_server_port.getValue());
  sprintf(mqtt.dev_short, "%s", WiFiManager_mqtt_client_short.getValue());
  Serial.println(("=== Saving parameters: ==="));
  Serial.print(("mqtt ip: ")); Serial.println(mqtt.server_ip);
  Serial.print(("mqtt port: ")); Serial.println(mqtt.server_port);
  Serial.print(("mqtt user: ")); Serial.println(mqtt.login);
  Serial.print(("capabilities: ")); Serial.println(mqtt.cap);
  Serial.print(("mqtt pw: ")); Serial.println(mqtt.pw);
  Serial.print(("mqtt dev short: ")); Serial.println(mqtt.dev_short);
  Serial.println(("=== End of parameters ==="));
  wifiManager.storeMqttStruct((char*) &mqtt, sizeof(mqtt));
  Serial.println("Configuration saved, restarting");
  delay(2000);
  ESP.reset(); // eigentlich muss das gehen so, .. // we can't change from AP mode to client mode, thus: reboot
}

void loadConfig(){
  // fill the mqtt element with all the data from eeprom
  wifiManager.loadMqttStruct((char*) &mqtt, sizeof(mqtt));
  Serial.println(("=== Loaded parameters: ==="));
  wifiManager.explainMqttStruct(0, false);
  Serial.println(mqtt.login);
  wifiManager.explainMqttStruct(1, false);
  Serial.println(mqtt.pw);
  wifiManager.explainMqttStruct(2, false);
  Serial.println(mqtt.dev_short);
  wifiManager.explainMqttStruct(3, false);
  Serial.println(mqtt.cap);
  wifiManager.explainMqttStruct(4, false);
  Serial.println(mqtt.server_ip);
  wifiManager.explainMqttStruct(5, false);
  Serial.println(mqtt.server_port);

  // capabilities
  if(((uint8_t)(mqtt.cap[0])-'0') & NEOPIXEL_BITMASK){
    m_use_neo_as_rgb = true;
    Serial.println(" + NeoPixel light");
  } else {
    m_use_neo_as_rgb = false;
    Serial.println(" - NeoPixel light");
  }
  if(((uint8_t)(mqtt.cap[0])-'0') & AVOID_RELAY_BITMASK){
    m_avoid_relay = true;
    Serial.println(" + avoid relay");
  } else {
    m_avoid_relay = false;
    Serial.println(" - avoid relay");
  }
  // capabilities

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
  Serial.print("Startup v");
  Serial.println(VERSION);
  Serial.print("Pinout ");
  Serial.println(PINOUT);
  if(ESP.getFlashChipRealSize()!=ESP.getFlashChipSize()){
    Serial.println("============================");
    Serial.println("warning, wrong flash config");
    Serial.print("Dev has ");
    Serial.println(ESP.getFlashChipRealSize());
    Serial.println("============================");
  }
  Serial.println("Flash config correct");

  // init the led
  pinMode(PWM_LIGHT_PIN1, OUTPUT);
  pinMode(PWM_LIGHT_PIN2, OUTPUT);
  pinMode(PWM_LIGHT_PIN3, OUTPUT);
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
  wifiManager.setMqtt(&mqtt); // save to reuse structure (only to save memory)
  WiFi.mode(WIFI_STA); // avoid station and ap at the same time

  if(digitalRead(BUTTON_INPUT_PIN)==LOW){
    wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
  };
  //wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!

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

  // ws strip, only if configured
  if(m_use_neo_as_rgb){
    strip.Begin();
    setAnimationType(ANIMATION_OFF); // important, otherwise they will be initialized half way on or strange colored
  };

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
	      	if(timer_dimmer+m_pwm_dimm_time > timer_dimmer_end){
	        	m_light_current = m_light_target;
	      	} else {
	  			m_light_current.r = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.r, m_light_target.r);
	  			m_light_current.g = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.g, m_light_target.g);
	  			m_light_current.b = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.b, m_light_target.b);
	      	}

			//Serial.println(m_light_target.r);
			setPWMLightState(true); // with override

		}
	}
	//// dimming end ////

  //// RGB circle ////
  if(m_animation_type){
    if(millis() >= m_neopixel_dimm_time) {
      if(m_animation_type == ANIMATION_RAINBOW_WHEEL){
        for(int i=0; i<NEOPIXEL_LED_COUNT; i++) {
          strip.SetPixelColor(i,Wheel(((i * 256 / NEOPIXEL_LED_COUNT) + m_animation_pos) & 255));
        }
        m_animation_pos++; // move wheel by one, will overrun and therefore cycle
        strip.Show();
        m_neopixel_dimm_time = millis() + NEOPIXEL_STEP_TIME; // schedule update
      }
      else if(m_animation_type == ANIMATION_RAINBOW_SIMPLE){
        for(int i=0; i<NEOPIXEL_LED_COUNT; i++) {
          strip.SetPixelColor(i,Wheel(m_animation_pos & 255));
        }
        m_animation_pos++; // move wheel by one, will overrun and therefore cycle
        strip.Show();
        m_neopixel_dimm_time = millis() + 4*NEOPIXEL_STEP_TIME; // schedule update
      }
      else if(m_animation_type == ANIMATION_COLOR_WIPE){
        // m_animation_pos geht bis 255/25 pixel = 10 colors,das wheel hat 255 pos, also 25 per 10 .. heh?
        strip.SetPixelColor(m_animation_pos % NEOPIXEL_LED_COUNT,Wheel(int(m_animation_pos/NEOPIXEL_LED_COUNT)*76 & 255)); // max: 255/10=25,25*10 to scale, 250*3 *3 will jump to various colors
        m_animation_pos++; // move wheel by one, will overrun and therefore cycle
        strip.Show();
        m_neopixel_dimm_time = millis() + 3*NEOPIXEL_STEP_TIME; // schedule update
      }
    } // timer
  } // if active
  //// RGB circle ////

	//// publish all state - ONLY after being connected for sure ////

	if (m_simple_light_state != m_published_simple_light_state) {
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishSimpleLightState();
      timer_republish_avoid = millis();
			timer_last_publish = millis();
		}
	}

if (m_button_press != m_published_button_press) {
    if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
      publishButtonPush();
      timer_republish_avoid = millis();
      timer_last_publish = millis();
    }
  }

	if (m_pwm_light_state != m_published_pwm_light_state) {
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishPWMLightState();
      timer_republish_avoid = millis();
			timer_last_publish = millis();
		};
	}

	if (m_published_light_brightness != m_light_target.r) { // todo .. fails to publish if we publish to much at the first run ...
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishPWMLightBrightness();
			timer_last_publish = millis();
		}
	}

	if (m_published_light_color != m_light_target.r + m_light_target.g + m_light_target.b){
		if(millis()-timer_last_publish > PUBLISH_TIME_OFFSET){
			publishPWMRGBColor();
			timer_last_publish = millis();
		}
	}

	if (m_pir_state != m_published_pir_state) {
		if(millis()-timer_last_publish >PUBLISH_TIME_OFFSET){
			publishPirState();
			timer_last_publish = millis();
		}
	}

  if(m_use_neo_as_rgb & m_published_animation_type != m_animation_type){
    if(millis()-timer_last_publish >PUBLISH_TIME_OFFSET){
      publishAnimationType();
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
			digitalWrite(GPIO_D8,HIGH); // prepare for next step ... 6sec on / 100ms would be enough ..
		} else if(periodic_slot == 4){
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
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}