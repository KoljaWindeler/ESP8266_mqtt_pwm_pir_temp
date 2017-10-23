/*
 * Highly based on a combination of different version of
 * https://github.com/mertenats/Open-Home-Automation/tree/master/ha_mqtt_binary_sensor_pir
 * Also using https://github.com/tzapu/WiFiManager
 *
 * Configuration (HA) :
 * light:
 *
 #######  single CHANNEL ###################
 ##############- platform: mqtt ### tower living
 ##############name: "dev1"
 ##############state_topic: "dev1/PWM_light/status"
 ##############command_topic: "dev1/PWM_dimm/switch"
 ##############brightness_state_topic: 'dev1/PWM_light/brightness'
 ## use the line below for dimming
 ####brightness_command_topic: 'dev1/PWM_dimm/brightness/set'
 ## use the line below for setting hard
 # brightness_command_topic: 'dev1/PWM_light/brightness/set'
 # qos: 0
 # payload_on: "ON"
 # payload_off: "OFF"
 # optimistic: false
 # brightness_scale: 99
 #######  single CHANNEL ###################
 #######  rgb CHANNEL ###################
 ##############- platform: mqtt ### tower living
 ##############name: "dev1"
 ##############state_topic: "dev1/PWM_light/status"
 ##############command_topic: "dev1/PWM_dimm/switch"
 ##############rgb_state_topic: 'dev1/PWM_RGB_dimm/color/set'
 # use the lines below for rgb dimming
 # rgb_command_topic: 'dev1/PWM_RGB_dimm/color/set'
 # use the lines below for setting
 # rgb_command_topic: 'dev1/PWM_RGB_light/color/set'
 # qos: 0
 # payload_on: "ON"
 # payload_off: "OFF"
 # optimistic: false
 #######  rgb CHANNEL ###################
 #######
 #######
 ##############config for sonoff modules:
 ##############generic 8266
 ##############DIO flash mode
 ##############flash 40 mhz, cpu 80mhz
 ##############flash size 1mb, 64k filesystem
 #######
 ##############config for sonoff touch modules:
 ##############generic 8266
 ##############DOUT flash mode!
 ##############flash 40 mhz, cpu 80mhz
 ##############flash size 1mb, 64k filesystem
 #######
 ##############QIO > QOUT > DIO > DOUT.
 #######
 ##############This sketch (2017/01/21): 333k (Basic wifi sketch 230k)
 #######
 ##############requires arduino libs:
 ##############- adafruit unified sensor
 ##############- adafruit dht22
 ##############- onewire
 */

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // local modified version          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <NeoPixelBus.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DHT.h>
#include <Wire.h>
#include <Arduino.h>
#include <ESP8266httpUpdate.h>
#include <my9291.h>

#define FS(x) (__FlashStringHelper *) (x)

// ////////////////////////////////////////////////// defines //////////////////////////////////////////////
#define SERIAL_DEBUG
#define DIMM_DONE    0 // state defs
#define DIMM_DIMMING 1 // state defs
#define DHT_def      1
#define DS_def       2
#define CONFIG_SSID  "ESP_CONFIG" // SSID of the configuration mode
#define MAX_AP_TIME  300                   // restart eps after 300 sec in config mode
#define MAX_CON_TIME 30										// give up connecting after 30 sec
#define TEMP_MAX     70                    // DS18B20 repoorts 85.0 on first reading ... for whatever reason
#define VERSION      "20171022"

// capability list
#define RGB_PWM_BITMASK     1 << 0 // 1
#define NEOPIXEL_BITMASK    1 << 1 // 2
#define AVOID_RELAY_BITMASK 1 << 2 // 4
#define SONOFF_B1_BITMASK   1 << 3 // 8
#define AITINKER_BITMASK    1 << 4 // 16
//#define MY9291_BITMASK      1 << 5 // 32
//#define MY9291_BITMASK      1 << 6 // 64
//#define MY9291_BITMASK      1 << 7 // 128

// pins
#define PINOUT_SONOFF       1 // set this to "#define" for the sonoff and pcb v3 but not v2
// #define PINOUT_KOLJA_TINY 1
// #define PINOUT_KOLJA_V2         // set this to "#define" for the pcb firmware .. incosistent pinout
// D8 war nicht so gut ... startet nicht mehr
#ifdef PINOUT_SONOFF
	# define PINOUT           "SONOFF"
	# define SIMPLE_LIGHT_PIN 12 // D6
	# define DS_PIN           13 // D7
	# define PIR_PIN          14 // D5
	# define PWM_LIGHT_PIN1   4  // D2
	# define PWM_LIGHT_PIN2   5  // D1
	# define PWM_LIGHT_PIN3   16 // D0
	# define BUTTON_INPUT_PIN 0  // D3
	# define DHT_PIN          2  // D4
	# define GPIO_D8          15 // D8
#endif // ifdef PINOUT_SONOFF
#ifdef PINOUT_KOLJA_V2
	# define PINOUT           "Kolja_v2"
	# define SIMPLE_LIGHT_PIN 13 // D7
	# define DS_PIN           12 // D6
	# define PIR_PIN          5  // D1
	# define PWM_LIGHT_PIN1   4  // D2
	# define PWM_LIGHT_PIN2   5  // D1
	# define PWM_LIGHT_PIN3   16 // D0
	# define BUTTON_INPUT_PIN 0  // D3
	# define DHT_PIN          2  // D4
	# define GPIO_D8          15 // D8
#endif // ifdef PINOUT_KOLJA_V2
#ifdef PINOUT_KOLJA_TINY
	# define PINOUT           "TINY"
	# define SIMPLE_LIGHT_PIN 12
	# define DS_PIN           14
	# define PIR_PIN          13
	# define PWM_LIGHT_PIN1   4
	# define PWM_LIGHT_PIN2   5
	# define PWM_LIGHT_PIN3   16
	# define BUTTON_INPUT_PIN 0
	# define DHT_PIN          2
	# define GPIO_D8          15
#endif // ifdef PINOUT_KOLJA_TINY


#define MY9291_DI_PIN	12	// mtdi 12
#define MY9291_DCKI_PIN	14 // mtms gpio 14?
#define MY9291_CHANNELS	6 // r,g,b,warm white, cold white

#define BUTTON_TIMEOUT         1500 // max 1500ms timeout between each button press to count up (start of config)
#define BUTTON_DEBOUNCE        400  // ms debouncing for the botton
#define MSG_BUFFER_SIZE        60   // mqtt messages max char size
#define TOPIC_BUFFER_SIZE			 35		// mqtt topic buffer
#define REPUBISH_AVOID_TIMEOUT 2000 // if the device was switched off, it will pubish its new state (and thats good)
                                    // that can retrigger another "switch off" to the same device if it's in group.
                                    // We have to avoid that we send our state again to break this loop
                                    // so we're going to suppress further status reports during this timeout

#define TOTAL_PERIODIC_SLOTS     5                            // 5 sensors: adc, rssi, dht_hum, dht_temp, ds18b20
#define UPDATE_PERIODIC          60000 / TOTAL_PERIODIC_SLOTS // update temperature once per minute .. but with 5 sensors we have to be faster
#define PUBLISH_TIME_OFFSET      200                          // ms timeout between two publishes
#define UPDATE_PIR               900000L                      // ms timeout between two publishes of the pir .. needed?

#define ANIMATION_STEP_TIME      15 // 256 steps per rotation * 15 ms/step = 3.79 sec pro rot
#define NEOPIXEL_LED_COUNT       24
#define ANIMATION_OFF            0
#define ANIMATION_RAINBOW_WHEEL  1
#define ANIMATION_RAINBOW_SIMPLE 2
#define ANIMATION_COLOR_WIPE     3


struct led {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

boolean publishPWMLightState();
boolean publishPWMLightBrightness();
boolean publishPWMRGBColor();
boolean publishSimpleLightState();
boolean publishPirState();
boolean publishButtonPush();
boolean publishAnimationType();
boolean publishTemperature(float temp, int DHT_DS);
boolean publishHumidity(float hum);
boolean publishRssi(float rssi);
boolean publishADC(int adc);
void callback(char * p_topic, byte * p_payload, unsigned int p_length);
void setPWMLightState();
void setPWMLightState(boolean over_ride);
void setSimpleLightState();
void setAnimationType(int type);
void pwmDimmTo(led dimm_to);
float getDsTemp();
float getDhtTemp();
float getDhtHumidity();
void updatePIRstate();
void updateBUTTONstate();
void reconnect();
void configModeCallback(WiFiManager * myWiFiManager);
void saveConfigCallback();
void loadConfig();
char * build_topic(const char * topic);

void setup();
void loop();
RgbColor Wheel(byte WheelPos);
