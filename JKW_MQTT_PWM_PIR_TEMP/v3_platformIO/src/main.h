/*
 * Highly based on a combination of different version of
 * https://github.com/mertenats/Open-Home-Automation/tree/master/ha_mqtt_binary_sensor_pir
 * Also using https://github.com/tzapu/WiFiManager
 *
 * Configuration (HA) :
 * light:
 *
 #######  single CHANNEL ###################
 ############################- platform: mqtt ### tower living
 ############################name: "dev1"
 ############################state_topic: "dev1/PWM_light/status"
 ############################command_topic: "dev1/PWM_dimm/switch"
 ############################brightness_state_topic: 'dev1/PWM_light/brightness'
 ## use the line below for dimming
 ########brightness_command_topic: 'dev1/PWM_dimm/brightness/set'
 ## use the line below for setting hard
 # brightness_command_topic: 'dev1/PWM_light/brightness/set'
 # qos: 0
 # payload_on: "ON"
 # payload_off: "OFF"
 # optimistic: false
 # brightness_scale: 99
 #######  single CHANNEL ###################
 #######  rgb CHANNEL ###################
 ############################- platform: mqtt ### tower living
 ############################name: "dev1"
 ############################state_topic: "dev1/PWM_light/status"
 ############################command_topic: "dev1/PWM_dimm/switch"
 ############################rgb_state_topic: 'dev1/PWM_RGB_dimm/color/set'
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
 ############################config for sonoff modules:
 ############################generic 8266
 ############################DIO flash mode
 ############################flash 40 mhz, cpu 80mhz
 ############################flash size 1mb, 64k filesystem
 #######
 ############################config for sonoff touch modules:
 ############################generic 8266
 ############################DOUT flash mode!
 ############################flash 40 mhz, cpu 80mhz
 ############################flash size 1mb, 64k filesystem
 #######
 ############################QIO > QOUT > DIO > DOUT.
 #######
 ############################This sketch (2017/01/21): 333k (Basic wifi sketch 230k)
 #######
 ############################requires arduino libs:
 ############################- adafruit unified sensor
 ############################- adafruit dht22
 ############################- onewire
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
#include "mqtt_parameter.h"
#include "logging.h"

#define FS(x) (__FlashStringHelper *) (x)

// ////////////////////////////////////////////////// defines //////////////////////////////////////////////
#define SERIAL_DEBUG
#define DIMM_DONE             0 // state defs
#define DIMM_DIMMING          1 // state defs
#define DHT_def               1
#define DS_def                2
#define TEMP_MAX              70           // DS18B20 repoorts 85.0 on first reading ... for whatever reason
#define VERSION               "20171030"

#define CONFIG_SSID           "ESP_CONFIG" // SSID of the configuration mode
#define MAX_CON_TIME          15           // give up connecting after 15 sec per try
#define MIN_RECONNECT_TIME    45           // try to reconnect for at least 45 sec, even after power up
#define CALC_RECONNECT_WEIGHT 5            // add 1 sec of reconnect time per 5 sec that we've been connected previously
#define MAX_RECONNECT_TIME    20 * 60      // even if we've been happliy connected to this network for weeks: start AP after 20 min of trying to reconnect
#define MAX_AP_TIME           300          // close AP after 300 sec in config mode and try reconnect


// capability list
#define RGB_PWM_BITMASK     1 << 0 // 1
#define NEOPIXEL_BITMASK    1 << 1 // 2
#define AVOID_RELAY_BITMASK 1 << 2 // 4
#define SONOFF_B1_BITMASK   1 << 3 // 8
#define AITINKER_BITMASK    1 << 4 // 16
// #define MY9291_BITMASK      1 << 5 // 32
// #define MY9291_BITMASK      1 << 6 // 64
// #define MY9291_BITMASK      1 << 7 // 128

// pins
#define PINOUT_SONOFF 1 // set this to "#define" for the sonoff and pcb v3 but not v2
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


#define MY9291_DI_PIN          12 // mtdi 12
#define MY9291_DCKI_PIN        14 // mtms gpio 14?
#define MY9291_CHANNELS        6  // r,g,b,warm white, cold white

#define BUTTON_TIMEOUT         1500 // max 1500ms timeout between each button press to count up (start of config)
#define BUTTON_DEBOUNCE        400  // ms debouncing for the botton
#define MSG_BUFFER_SIZE        60   // mqtt messages max char size
#define TOPIC_BUFFER_SIZE      35   // mqtt topic buffer
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

mqtt_parameter_8 m_pwm_light_state;
mqtt_parameter_8 m_simple_light_state;
mqtt_parameter_8 m_button_press;
mqtt_parameter_8 m_light_brightness;
mqtt_parameter_8 m_pir_state;
mqtt_parameter_8 m_animation_type; // type 1 = rainbow wheel, 2 = simple rainbow .. see define above
mqtt_parameter_16 m_light_color;

logging logger;


// MQTT: topics, constants, etc
static constexpr char MQTT_PWM_LIGHT_STATE_TOPIC[]   = "/PWM_light/status"; // publish state here ON / OFF
static constexpr char MQTT_PWM_LIGHT_COMMAND_TOPIC[] = "/PWM_light/switch"; // get command here ON / OFF

static constexpr char MQTT_SIMPLE_LIGHT_STATE_TOPIC[]   = "/simple_light/status"; // publish state here ON / OFF
static constexpr char MQTT_SIMPLE_LIGHT_COMMAND_TOPIC[] = "/simple_light/switch"; // get command here ON / OFF

static constexpr char MQTT_RAINBOW_STATUS_TOPIC[]         = "/rainbow/status";        // publish state here ON / OFF
static constexpr char MQTT_RAINBOW_COMMAND_TOPIC[]        = "/rainbow/switch";        // get command here ON / OFF
static constexpr char MQTT_SIMPLE_RAINBOW_STATUS_TOPIC[]  = "/simple_rainbow/status"; // publish state here ON / OFF
static constexpr char MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC[] = "/simple_rainbow/switch"; // get command here ON / OFF
static constexpr char MQTT_COLOR_WIPE_STATUS_TOPIC[]      = "/color_wipe/status";     // publish state here ON / OFF
static constexpr char MQTT_COLOR_WIPE_COMMAND_TOPIC[]     = "/color_wipe/switch";     // get command here ON / OFF

static constexpr char MQTT_PWM_LIGHT_BRIGHTNESS_STATE_TOPIC[]   = "/PWM_light/brightness";     // publish 0-99
static constexpr char MQTT_PWM_LIGHT_BRIGHTNESS_COMMAND_TOPIC[] = "/PWM_light/brightness/set"; // set value 0-99
static constexpr char MQTT_PWM_DIMM_COMMAND_TOPIC[] = "/PWM_dimm/switch";                      // get ON/OFF command here

static constexpr char MQTT_PWM_RGB_DIMM_COLOR_STATE_TOPIC[]   = "/PWM_RGB_light/color";     // publish value "0-99,0-99,0-99"
static constexpr char MQTT_PWM_RGB_COLOR_COMMAND_TOPIC[]      = "/PWM_RGB_light/color/set"; // set value "0-99,0-99,0-99"
static constexpr char MQTT_PWM_RGB_DIMM_COLOR_COMMAND_TOPIC[] = "/PWM_RGB_dimm/color/set";  // set value "0-99,0-99,0-99"

static constexpr char MQTT_PWM_DIMM_DELAY_COMMAND_TOPIC[]      = "/PWM_dimm/delay/set";      // set value
static constexpr char MQTT_PWM_DIMM_BRIGHTNESS_COMMAND_TOPIC[] = "/PWM_dimm/brightness/set"; // set value

static constexpr char MQTT_MOTION_STATUS_TOPIC[]   = "/motion/status";   // publish
static constexpr char MQTT_TEMPARATURE_DS_TOPIC[]  = "/temperature_DS";  // publish
static constexpr char MQTT_TEMPARATURE_DHT_TOPIC[] = "/temperature_DHT"; // publish
static constexpr char MQTT_HUMIDITY_DHT_TOPIC[]    = "/humidity_DHT";    // publish
static constexpr char MQTT_BUTTON_TOPIC[]     = "/button";               // publish
static constexpr char MQTT_RSSI_STATE_TOPIC[] = "/rssi";                 // publish
static constexpr char MQTT_ADC_STATE_TOPIC[]  = "/adc";                  // publish
static constexpr char MQTT_SETUP_TOPIC[]      = "/setup";                // subscribe

static constexpr char STATE_ON[]  = "ON";
static constexpr char STATE_OFF[] = "OFF";

led m_light_target = { 99, 99, 99 };
led m_light_start = { 99, 99, 99 };
led m_light_current = { 99, 99, 99 }; // actual value written to PWM
led m_light_backup = { 99, 99, 99 };  // to be able to resume "dimm on" to the last set color

// converts from half log to linear .. the human eye is a smartass
const uint8_t intens[100] =
{ 0,     1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
	 23,   24,  25,  26,  27,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  38,  39,  40,  41,  43,  44,  45,  47,
	 48,   50,  51,  53,  55,  57,  58,  60,  62,  64,  66,  68,  70,  73,  75,  77,  80,  82,  85,  88,  91,  93,  96,
	 99,  103, 106, 109, 113, 116, 120, 124, 128, 132, 136, 140, 145, 150, 154, 159, 164, 170, 175, 181, 186, 192, 198,
	 205, 211, 218, 225, 232, 239, 247, 255 };

const uint8_t hb[34] = { 27, 27, 27, 27, 27, 27, 17, 27, 37, 21, 27, 27, 27, 27, 27, 52, 71,
	                        91, 99, 91, 33,  0, 12, 29, 52, 33, 21, 26, 33, 26, 20, 27, 27, 27 };

uint16_t m_pwm_dimm_time = 10;    // 10ms per Step, 255*0.01 = 2.5 sec
bool m_use_neo_as_rgb    = false; // if true we're going to send the color to the neopixel and not to the pwm pins
bool m_use_my92x1_as_rgb = false; // for b1 bubble / aitinker
bool m_use_pwm_as_rgb    = false; // for mosfet pwm
bool m_avoid_relay       = false; // to avoid clicking relay
uint8_t m_animation_pos  = 0;     // pointer im wheel


// buffer used to send/receive data with MQTT, can not be done with the m_topic_buffer, as both as need simultaniously
char m_topic_buffer[TOPIC_BUFFER_SIZE];
char m_msg_buffer[MSG_BUFFER_SIZE];

WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient client(wifiClient);
mqtt_data mqtt;

// prepare wifimanager variables
WiFiManagerParameter WiFiManager_mqtt_server_ip("mq_ip", "mqtt server ip", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_port("mq_port", "mqtt server port", "1883", 5);
WiFiManagerParameter WiFiManager_mqtt_capability_b0("cap_0", "PWM Leds", 0, 2, true);       // length must be at least tw0 .. why? // Capability Bit0 = PWM LEDs connected
WiFiManagerParameter WiFiManager_mqtt_capability_b1("cap_1", "Neopixel", 0, 2, true);       // Bit1 = Neopixel,
WiFiManagerParameter WiFiManager_mqtt_capability_b2("cap_2", "Avoid relay", 0, 2, true);    // Bit2 = Avoid relay
WiFiManagerParameter WiFiManager_mqtt_capability_b3("cap_3", "Sonoff B1", 0, 2, true);      // Bit3 = sonoff b1
WiFiManagerParameter WiFiManager_mqtt_capability_b4("cap_4", "AiTinker light", 0, 2, true); // Bit4 = aitinker
WiFiManagerParameter WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 6);
WiFiManagerParameter WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);


uint8_t counter_button         = 0;
uint8_t periodic_slot          = 0;
uint32_t updateFastValuesTimer = 0;
uint32_t updateSlowValuesTimer = 0;
uint32_t timer_dimmer          = 0;
uint32_t timer_dimmer_start    = 0;
uint32_t timer_dimmer_end      = 0;
uint32_t timer_republish_avoid = 0;
uint32_t timer_button_down     = 0;
uint32_t timer_connected_start = 0;
uint32_t timer_connected_stop  = 0;
uint32_t timer_last_publish    = 0;
uint32_t m_animation_dimm_time = 0;
