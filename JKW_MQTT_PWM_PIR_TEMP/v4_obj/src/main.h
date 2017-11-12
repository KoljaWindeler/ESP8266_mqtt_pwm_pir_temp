/*
 * Highly based on a combination of different version of
 * https://github.com/mertenats/Open-Home-Automation/tree/master/ha_mqtt_binary_sensor_pir
 * Also using https://github.com/tzapu/WiFiManager
 *
 * Configuration (HA) :
 * light:
 *
 #######  single CHANNEL ###################
 ###################################- platform: mqtt ### tower living
 ###################################name: "dev1"
 ###################################state_topic: "dev1/PWM_light/status"
 ###################################command_topic: "dev1/PWM_dimm/switch"
 ###################################brightness_state_topic: 'dev1/PWM_light/brightness'
 ## use the line below for dimming
 ############brightness_command_topic: 'dev1/PWM_dimm/brightness/set'
 ## use the line below for setting hard
 # brightness_command_topic: 'dev1/PWM_light/brightness/set'
 # qos: 0
 # payload_on: "ON"
 # payload_off: "OFF"
 # optimistic: false
 # brightness_scale: 99
 #######  single CHANNEL ###################
 #######  rgb CHANNEL ###################
 ###################################- platform: mqtt ### tower living
 ###################################name: "dev1"
 ###################################state_topic: "dev1/PWM_light/status"
 ###################################command_topic: "dev1/PWM_dimm/switch"
 ###################################rgb_state_topic: 'dev1/PWM_RGB_dimm/color/set'
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
 ###################################config for sonoff modules:
 ###################################generic 8266
 ###################################DIO flash mode
 ###################################flash 40 mhz, cpu 80mhz
 ###################################flash size 1mb, 64k filesystem
 #######
 ###################################config for sonoff touch modules:
 ###################################generic 8266
 ###################################DOUT flash mode!
 ###################################flash 40 mhz, cpu 80mhz
 ###################################flash size 1mb, 64k filesystem
 #######
 ###################################QIO > QOUT > DIO > DOUT.
 #######
 ###################################This sketch (2017/01/21): 333k (Basic wifi sketch 230k)
 #######
 ###################################requires arduino libs:
 ###################################- adafruit unified sensor
 ###################################- adafruit dht22
 ###################################- onewire
 */

#ifndef main_h
	#define main_h

	#define FS(x) (__FlashStringHelper *) (x)

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // local modified version          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <NeoPixelBus.h>

#include "mqtt_parameter.h"
#include "logging.h"
#include "capability.h"


	class peripheral
	{
public:
		peripheral(){ }

		virtual ~peripheral(){ }

		virtual bool init() = 0;
		virtual bool loop() = 0;
		virtual bool intervall_update(uint8_t slot) = 0;
		virtual uint8_t count_intervall_update()    = 0;
		virtual bool subscribe() = 0;
		virtual bool receive(uint8_t * p_topic, uint8_t * p_payload) = 0;
		virtual bool parse(uint8_t * config) = 0;
		virtual uint8_t * get_key() = 0;
		virtual void interrupt()    = 0;
		virtual bool publish()      = 0;
	};

	// ////////////////////////////////////////////////// defines //////////////////////////////////////////////
	#define SERIAL_DEBUG
	#define DIMM_DONE             0 // state defs
	#define DIMM_DIMMING          1 // state defs
	#define DHT_def               1
	#define DS_def                2
	#define TEMP_MAX              70 // DS18B20 repoorts 85.0 on first reading ... for whatever reason
	#define VERSION               "20171111_2"

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

	#define T_SL   1
	#define T_PWM  2
	#define T_NEO  3
	#define T_BOne 4
	#define T_AI   5

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
		# define PWM_LIGHT_PIN2   4  // D1
		# define PWM_LIGHT_PIN3   4  // D0
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


	#define BUTTON_TIMEOUT         1500 // max 1500ms timeout between each button press to count up (start of config)
	#define BUTTON_DEBOUNCE        400  // ms debouncing for the botton
	#define MSG_BUFFER_SIZE        60   // mqtt messages max char size
	#define TOPIC_BUFFER_SIZE      64   // mqtt topic buffer
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

	#define MAX_PERIPHERALS 20


	struct led {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	void callback(char * p_topic, byte * p_payload, unsigned int p_length);
	void reconnect();
	void configModeCallback(WiFiManager * myWiFiManager);
	void saveConfigCallback();
	void loadConfig();
	char * build_topic(const char * topic);
	void loadPheripherals(uint8_t * peripherals);
	void setup();
	void loop();


	/*mqtt_parameter_8 m_simple_light_state;
	 * mqtt_parameter_8 m_button_press;
	 * mqtt_parameter_8 m_light_brightness;
	 *
	 * mqtt_parameter_8 m_animation_type; // type 1 = rainbow wheel, 2 = simple rainbow .. see define above
	 * mqtt_parameter_16 m_light_color;
	 */

	// MQTT: topics, constants, etc, send and receive are written from the PC perspective
	//  setter aka the topics we'll subscribe to
	// light direct
	static constexpr char MQTT_LIGHT_COMMAND_TOPIC[]            = "/s/light";            // get command here ON / OFF
	static constexpr char MQTT_LIGHT_COLOR_COMMAND_TOPIC[]      = "/s/light/color";      // set value "0-99,0-99,0-99"  will switch hard to the value
	static constexpr char MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC[] = "/s/light/brightness"; // set value 0-99 will switch hard to the value
	// ligth dimm
	static constexpr char MQTT_LIGHT_DIMM_COMMAND_TOPIC[] = "/s/light/dimm";                      // get ON/OFF command here
	static constexpr char MQTT_LIGHT_DIMM_BRIGHTNESS_COMMAND_TOPIC[] = "/s/light/dimm/brightness"; // set value, will dimm towards the new value
	static constexpr char MQTT_LIGHT_DIMM_DELAY_COMMAND_TOPIC[]      = "/s/light/dimm/delay";     // set value, will dimm towards the new value
	static constexpr char MQTT_LIGHT_DIMM_COLOR_COMMAND_TOPIC[]      = "/s/light/dimm/color";     // set value "0-99,0-99,0-99", will dimm towards the new value
	// animation command
	static constexpr char MQTT_LIGHT_ANIMATION_RAINBOW_COMMAND_TOPIC[]        = "/s/light/animation/rainbow";        // get command here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_COMMAND_TOPIC[] = "/s/light/animation/simple_rainbow"; // get command here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_COLOR_WIPE_COMMAND_TOPIC[]     = "/s/light/animation/color_wipe";     // get command here ON / OFF
	// light states
	static constexpr char MQTT_LIGHT_STATE_TOPIC[]            = "/r/light";            // publish state here ON / OFF
	static constexpr char MQTT_LIGHT_COLOR_STATE_TOPIC[]      = "/r/light/color";      // publish value "0-99,0-99,0-99"
	static constexpr char MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC[] = "/r/light/brightness"; // publish 0-99
	// animation states
	static constexpr char MQTT_LIGHT_ANIMATION_RAINBOW_STATUS_TOPIC[]        = "/r/light/animation/rainbow";        // publish state here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_STATUS_TOPIC[] = "/r/light/animation/simple_rainbow"; // publish state here ON / OFF
	static constexpr char MQTT_LIGHT_ANIMATION_COLOR_WIPE_STATUS_TOPIC[]     = "/r/light/animation/color_wipe";     // publish state here ON / OFF
	// misc
	static constexpr char MQTT_MOTION_STATUS_TOPIC[] = "/r/motion";      // publish
	static constexpr char MQTT_TEMPARATURE_TOPIC[]   = "/r/temperature"; // publish
	static constexpr char MQTT_HUMIDITY_TOPIC[]      = "/r/humidity";    // publish
	static constexpr char MQTT_BUTTON_TOPIC[]        = "/r/button";      // publish
	static constexpr char MQTT_RSSI_STATE_TOPIC[]    = "/r/rssi";        // publish
	static constexpr char MQTT_ADC_STATE_TOPIC[]     = "/r/adc";         // publish

	static constexpr char MQTT_SETUP_TOPIC[] = "/sr/setup"; // subscribe
	static constexpr char MQTT_CAPABILITY_TOPIC[] = "/sr/capability"; // subscribe
	static constexpr char MQTT_TRACE_COMMAND_TOPIC[] = "/s/trace"; // subscribe
	static constexpr char MQTT_TRACE_STATUS_TOPIC[] = "/r/trace"; // publish

	static constexpr char STATE_ON[]  = "ON";
	static constexpr char STATE_OFF[] = "OFF";


	extern WiFiClient wifiClient;
	extern WiFiManager wifiManager;
	extern PubSubClient client;
	extern mqtt_data mqtt;
	extern char m_topic_buffer[TOPIC_BUFFER_SIZE];
	extern char m_msg_buffer[MSG_BUFFER_SIZE];
	extern peripheral * p_adc;
	extern peripheral * p_animaton;
	extern peripheral * p_bOne;
	extern peripheral * p_button;
	extern peripheral * p_dimmer;
	extern peripheral * p_neo;
	extern peripheral * p_pir;
	extern peripheral * p_pwm;
	extern peripheral * p_rssi;
	extern peripheral * p_simple_light;
	extern peripheral * p_dht;
	extern peripheral * p_ds;
	extern peripheral * p_ai;
	extern peripheral * p_bOne;
	extern peripheral * p_neo;
	extern peripheral * p_light;
	extern const uint8_t intens[100];


#include "ADC.h"
#include "PIR.h"
#include "button.h"
#include "simple_light.h"
#include "rssi.h"
#include "PWM.h"
#include "J_DHT22.h"
#include "J_DS.h"
#include "AI.h"
#include "BOne.h"
#include "NeoStrip.h"
#include "light.h"

#endif // ifndef main_h
