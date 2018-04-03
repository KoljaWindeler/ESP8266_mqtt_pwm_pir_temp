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

	#define FS(x) (__FlashStringHelper *) (x)

#include <ESP8266WiFi.h>
#include <WiFiManager.h> // local modified version          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <NeoPixelBus.h>

#include "mqtt_parameter.h"
#include "logging.h"
#include "capability.h"
#include "connection_relay.h"


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
		virtual bool publish()      = 0;
	};

	// ////////////////////////////////////////////////// defines //////////////////////////////////////////////
	#define SERIAL_DEBUG
	#define DIMM_DONE             0 // state defs
	#define DIMM_DIMMING          1 // state defs
	#define DHT_def               1
	#define DS_def                2
	#define TEMP_MAX              70 // DS18B20 repoorts 85.0 on first reading ... for whatever reason
	#define DEV                   "" // set this to "_dev" during development to avoid Mesh confilicts
	#define VERSION               "20180403.2" DEV

	#define CONFIG_SSID           "ESP_CONFIG" // SSID of the configuration mode
	#define MAX_CON_TIME          25           // give up connecting after 25 sec per try
	#define MIN_RECONNECT_TIME    65           // try to reconnect for at least 65 sec, even after power up
	#define CALC_RECONNECT_WEIGHT 5            // add 1 sec of reconnect time per 5 sec that we've been connected previously
	#define MAX_RECONNECT_TIME    20 * 60      // even if we've been happliy connected to this network for weeks: start AP after 20 min of trying to reconnect
	#define MAX_AP_TIME           300          // close AP after 300 sec in config mode and try reconnect

	#define PINOUT                   "SONOFF"
	#define MSG_BUFFER_SIZE          60   // mqtt messages max char size
	#define TOPIC_BUFFER_SIZE        64   // mqtt topic buffer
	#define PUBLISH_TIME_OFFSET      200  // ms timeout between two publishes
	#define MAX_PERIPHERALS          20

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
	char * build_topic(const char * topic, uint8_t pc_shall_R_or_S, bool with_dev);
	void loadPheripherals(uint8_t * peripherals);
	void setup();
	void loop();
	bool bake(peripheral * p_obj, peripheral ** p_handle, uint8_t * config);

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

	static constexpr char STATE_ON[]  = "ON";
	static constexpr char STATE_OFF[] = "OFF";

	#define UNIT_TO_PC 'r'
	#define PC_TO_UNIT 's'

	extern WiFiClient wifiClient;
	extern WiFiManager wifiManager;
	extern PubSubClient client;
	extern mqtt_data mqtt;
	extern connection_relay network;
	extern char m_topic_buffer[TOPIC_BUFFER_SIZE];
	extern char m_msg_buffer[MSG_BUFFER_SIZE];
	extern peripheral * p_adc;
	extern peripheral * p_bOne;
	extern peripheral * p_button;
	extern peripheral * p_neo;
	extern peripheral * p_pir;
	extern peripheral * p_pir2;
	extern peripheral * p_pwm;
	extern peripheral * p_pwm2;
	extern peripheral * p_pwm3;
	extern peripheral * p_rssi;
	extern peripheral * p_simple_light;
	extern peripheral * p_dht;
	extern peripheral * p_ds;
	extern peripheral * p_ai;
	extern peripheral * p_bOne;
	extern peripheral * p_neo;
	extern peripheral * p_light;
	extern peripheral * p_hlw;
	extern peripheral * p_nl;
	extern peripheral * p_rfb;
	extern peripheral * p_gpio;
	extern peripheral * p_husqvarna;
	extern peripheral * p_no_mesh;


	extern const uint8_t intens[100];


#include "ADC.h"
#include "PIR.h"
#include "button.h"
#include "simple_light.h"
#include "rssi.h"
#include "PWM.h"
#include "J_DHT22.h"
#include "J_DS.h"
#include "J_hlw8012.h"
#include "AI.h"
#include "BOne.h"
#include "NeoStrip.h"
#include "light.h"
#include "night_light.h"
#include "bridge.h"
#include "J_GPIO.h"
#include "husqvarna.h"
#include "no_mesh.h"

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
 * 10	GPIO12		D6; MTDI; HSPI_MISO
 * 11	VDDPST		P	Digital / IO power supply (1.8 V ~ 3.3 V)
 * 12	GPIO13		D7; MTCK; HSPI_MOSI; UART0_CTS
 * 13	GPIO15		D8; MTDO; HSPI_CS; UART0_RTS
 * 14	GPIO02		D4; UART Tx during flash programming;
 * 15	GPIO00		D3; SPI_CS2
 * 16	GPIO04		D2
 *
 * 17	VDDPST		Digital / IO power supply (1.8 V ~ 3.3 V)
 * 18	GPIO09		SDIO_DATA_2; Connect to SD_D2 (Series R: 200 Ω); SPIHD; HSPIHD
 * 19	GPIO10		SDIO_DATA_3; Connect to SD_D3 (Series R: 200 Ω); SPIWP; HSPIWP
 * 20	GPIO11		SDIO_CMD;	Connect to SD_CMD (Series R: 200 Ω); SPI_CS0;
 * 21	GPIO06		SDIO_CLK; Connect to SD_CLK (Series R: 200 Ω); SPI_CLK;
 * 22	GPIO07		SDIO_DATA_0; Connect to SD_D0 (Series R: 200 Ω); SPI_MSIO;
 * 23	GPIO08		SDIO_DATA_1; Connect to SD_D1 (Series R: 200 Ω); SPI_MOSI;
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
