/*
 * for sonoff touch modules:
 * generic 8266
 * DOUT flash mode!
 * flash 40 mhz, cpu 80mhz
 * flash size 1mb, 64k filesystem
 *
 * QIO > QOUT > DIO > DOUT.
 *
 * Pin1 = 3.3V
 * Pin2 = RxD
 * Pin3 = TxD
 * Pin4 = Masse
 * Pin5 = vorbereitet für GPIO
 */

#ifndef main_h
	#define main_h

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // local modified version          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include "../lib/ESP8266httpUpdate/src/ESP8266httpUpdate.h"


#ifdef WITH_NEOSTRIP
	#include <NeoPixelBus.h>
#endif

#include "mqtt_parameter.h"
#include "logging.h"
#include "capability_parser.h"
#include "connection_relay.h"
#include "cap.h"


	// ////////////////////////////////////////////////// defines //////////////////////////////////////////////
	#define SERIAL_DEBUG
	#define DIMM_DONE             0 // state defs
	#define DIMM_DIMMING          1 // state defs
	#define DHT_def               1
	#define DS_def                2
	#define TEMP_MAX              70 // DS18B20 repoorts 85.0 on first reading ... for whatever reason
	#define DEV                   "" // set this to "_dev" during development to avoid Mesh confilicts
	#define VERSION               "20211222" DEV

	#define CONFIG_SSID           "ESP_CONFIG" // SSID of the configuration mode
	#define MAX_CON_TIME          25           // give up connecting after 25 sec per try
	#define MIN_RECONNECT_TIME    65           // try to reconnect for at least 65 sec, even after power up
	#define CALC_RECONNECT_WEIGHT 5            // add 1 sec of reconnect time per 5 sec that we've been connected previously
	#define MAX_RECONNECT_TIME    20 * 60      // even if we've been happliy connected to this network for weeks: start AP after 20 min of trying to reconnect
	#define MAX_AP_TIME           300          // close AP after 300 sec in config mode and try reconnect

	#define MSG_BUFFER_SIZE          60   // mqtt messages max char size
	#define TOPIC_BUFFER_SIZE        64   // mqtt topic buffer
	#define PUBLISH_TIME_OFFSET      200  // ms timeout between two publishes
	#define MAX_CAPS                 100

	struct led {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};

	void callback(char * p_topic, byte * p_payload, uint16_t p_length);
	void reconnect();
	void configModeCallback(WiFiManager * myWiFiManager);
	void saveConfigCallback();
	void loadConfig();
	char * build_topic(const char * topic, uint8_t pc_shall_R_or_S);
	char * build_topic(const char * topic, uint8_t max_pre_topic_length, uint8_t pc_shall_R_or_S, bool with_dev, ...);
	void loadPheripherals(uint8_t * peripherals);
	void setup();
	void loop();
	bool bake(capability * p_obj, capability ** p_handle, uint8_t * config);
	char* str_rpl(char* in, char old, char replacement);
	char* str_rpl(char* in, char old, char replacement, uint8_t len);
	char* discovery_topic_bake(const char* topic,...);

	// MQTT: topics, constants, etc, send and receive are written from the PC perspective
	//  setter aka the topics we'll subscribe to
	// all topics are used with /r/ or /s/ as prefix by the build_topic function
	// where /s/ topics are send fro, the PC and received here, and /r/ the otherway around
	// light direct

	// misc
	static constexpr char MQTT_TEMPARATURE_TOPIC[] = "temperature"; // publish
	static constexpr char MQTT_HUMIDITY_TOPIC[]    = "humidity";    // publish
	static constexpr char MQTT_SETUP_TOPIC[]       = "setup";       // subscribe
	static constexpr char MQTT_CAPABILITY_TOPIC[]  = "capability";  // subscribe
	static constexpr char MQTT_TRACE_TOPIC[]       = "trace";       // subscribe
	static constexpr char MQTT_NIGHT_LIGHT_TOPIC[] = "night";       // subscribe
	static constexpr char MQTT_OTA_TOPIC[]         = "ota";         // subscribe
	static constexpr char MQTT_CONFIG_LOCK_TOPIC[] = "config_lock"; // subscribe
	

	static constexpr char STATE_ON[]  = "ON";
	static constexpr char STATE_OFF[] = "OFF";

	#define UNIT_TO_PC 'r'
	#define PC_TO_UNIT 's'

	extern uint32_t timer_connected_start;
	extern WiFiClient wifiClient;
	extern WiFiManager wifiManager;
	extern PubSubClient client;
	extern mqtt_data mqtt;
	extern connection_relay network;
	extern char m_topic_buffer[TOPIC_BUFFER_SIZE];
	extern char m_msg_buffer[MSG_BUFFER_SIZE];
	extern capability * p_adc;
	extern capability * p_bOne;
	extern capability * p_button;
	extern capability * p_neo;
	extern capability * p_pir;
	extern capability * p_pwm;
	extern capability * p_pwm2;
	extern capability * p_pwm3;
	extern capability * p_shellyDimmer;
	extern capability * p_rssi;
	extern capability * p_simple_light;
	extern capability * p_remote_simple_light;
	extern capability * p_dht;
	extern capability * p_ds;
	extern capability * p_ai;
	extern capability * p_bOne;
	extern capability * p_neo;
	extern capability * p_light;
	extern capability * p_hlw;
	extern capability * p_nl;
	extern capability * p_rfb;
	extern capability * p_gpio;
	//extern capability * p_husqvarna;
	extern capability * p_no_mesh;
	extern capability * p_uptime;
	extern capability * p_play;
	extern capability * p_freq;
	extern capability * p_rec;
	extern capability * p_em;
	extern capability * p_em_bin;
	extern capability * p_ir;
	extern capability * p_SerSer;
	extern capability * p_ads1015;
	extern capability * p_hx711;
	extern capability * p_crash;
	extern capability * p_count;

	extern const uint8_t intens[100];


#include "cap_ADC.h"
#include "cap_PIR.h"
#include "cap_button.h"
#include "cap_simple_light.h"
#include "cap_remote_simple_light.h"
#include "cap_rssi.h"
#include "cap_PWM.h"
#include "cap_DHT22.h"
#include "cap_DS.h"
#include "cap_hlw8012.h"
#include "cap_AI.h"
#include "cap_BOne.h"
#include "cap_NeoStrip.h"
#include "cap_light.h"
#include "cap_night_light.h"
#include "cap_tuya_bridge.h"
#include "cap_GPIO.h"
//#include "cap_husqvarna.h"
#include "cap_mesh.h"
#include "cap_ir.h"
#include "cap_uptime.h"
#include "cap_play.h"
#include "cap_freq.h"
#include "cap_record.h"
#include "cap_energy_meter.h"
#include "cap_energy_meter_bin.h"
#include "cap_SerialServer.h"
#include "cap_shelly_dimmer.h"
#include "cap_ads1015.h"
#include "cap_hx711.h"
#include "cap_crash.h"
#include "cap_counter.h"

#endif // ifndef main_h

/*
 * 01	VDDA			Analog Power 3.0 V ~ 3.6V
 * 02	LNA				RF Antenna Interface Chip Output Impedance = 50 Ω No matching required. It is suggested to retain the π-type matching network to match the antenna.
 * 03	VDD3P3		Amplifier power: 3.0 V ~ 3.6 V
 * 04	VDD3P3		Amplifier power:3.0 V ~ 3.6 V
 * 05	VDD_RTC		NC (1.1 V)
 * 06	TOUT	I		ADC pin. It can be used to test the power-supply voltage of VDD3P3 (Pin3 and Pin4) and the input power voltage of TOUT (Pin 6). However, these two functions cannot be used simultaneously
 * 07	CHIP_EN		Chip Enable, High: On, chip works properly, Low: Off, small current consumed
 * 08	GPIO16		D0,XPD_DCDC, Deep-sleep wakeup (need to be connected to EXT_RSTB;
 *
 * 09	GPIO14		D5; MTMSl; HSPI_CLK
 * 10	GPIO12		D6; MTDI; HSPI_MISO 						// DO NOT USE, CONNECTED TO FLASH
 * 11	VDDPST		P	Digital / IO power supply (1.8 V ~ 3.3 V)
 * 12	GPIO13		D7; MTCK; HSPI_MOSI; UART0_CTS
 * 13	GPIO15		D8; MTDO; HSPI_CS; UART0_RTS  	// DO NOT USE, on board pull down
 * 14	GPIO02		D4; UART Tx during flash programming; internal LED
 * 15	GPIO00		D3; SPI_CS2; Button pin, needs to be high during startup, will be checked 10 sec afterboot, if low-> wifimanager start
 * 16	GPIO04		D2
 *
 * 17	VDDPST		Digital / IO power supply (1.8 V ~ 3.3 V)
 * 18	GPIO09		SDIO_DATA_2; Connect to SD_D2 (Series R: 200 Ω); SPIHD; HSPIHD // DO NOT USE, CONNECTED TO FLASH
 * 19	GPIO10		SDIO_DATA_3; Connect to SD_D3 (Series R: 200 Ω); SPIWP; HSPIWP // DO NOT USE, CONNECTED TO FLASH
 * 20	GPIO11		SDIO_CMD;	Connect to SD_CMD (Series R: 200 Ω); SPI_CS0;	// DO NOT USE, CONNECTED TO FLASH
 * 21	GPIO06		SDIO_CLK; Connect to SD_CLK (Series R: 200 Ω); SPI_CLK;	// DO NOT USE, CONNECTED TO FLASH
 * 22	GPIO07		SDIO_DATA_0; Connect to SD_D0 (Series R: 200 Ω); SPI_MSIO; // DO NOT USE, CONNECTED TO FLASH
 * 23	GPIO08		SDIO_DATA_1; Connect to SD_D1 (Series R: 200 Ω); SPI_MOSI; // DO NOT USE, CONNECTED TO FLASH
 * 24	GPIO05		D1
 *
 * 25 GPIO03		D9; U0RXD; UART Rx during flash programming;
 * 26	GPIO01		D10; U0TXD; UART Tx during flash programming; SPI_CS1
 * 27	XTAL_OUT	Connect to crystal oscillator output,	can be used to provide BT clock input
 * 28	XTAL_IN		Connect to crystal oscillator input
 * 29	VDDD			Analog power 3.0 V ~ 6 V
 * 30	VDDA			Analog power 3.0 V ~ 3.6 V
 * 31	RES12K		Serial connection with a 12 kΩ resistor to the ground
 * 32	EXT_RSTB	External reset signal (Low voltage level: Active)
 */


/*
 * Four pins on all SonOff devices
 * 1 3.3V
 * 2 RX of the ESp (tx of pc)
 * 3 TX of the ESP (rx of pc)
 * 4 GND
 */
