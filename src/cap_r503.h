#ifndef r503_h
#define r503_h

#include "main.h"
#include "Adafruit_Fingerprint.h"
#include "../lib/softserial/SoftwareSerial.h"


// These are the nominal values for the resistors in the circuit
// id 1 = kolja, links, Zeigefinger
// id 2 = carlo, links, Zeigefinger
// id 3 = kolja, links, Mittelfinger
// id 4 = Caro, links, Zeigefinger

#define R503_STATE_IDLE 0
#define R503_STATE_ENROLE_WAIT_FOR_FINGER_1 1 
#define R503_STATE_ENROLE_WAIT_FOR_LIFT 2
#define R503_STATE_ENROLE_WAIT_FOR_FINGER_2 3

static constexpr char MQTT_R503_COMMAND_TOPIC[] = "R503/command";
static constexpr char MQTT_R503_TOUCHRING_TOPIC[] = "R503/touchring";
static constexpr char MQTT_R503_DINGDONG_TOPIC[] = "R503/dingdong";
static constexpr char MQTT_R503_STATUS_TOPIC[] = "R503/status";
static constexpr char MQTT_R503_CONFIDENCE_TOPIC[] = "R503/confidence";
static constexpr char MQTT_R503_FINGER_ID_TOPIC[] = "R503/finger_id";
static constexpr char MQTT_R503_SILENT_TOPIC[] = "R503/silent";
static constexpr char MQTT_R503_COMMAND_LED_RED[] = "led_red";
static constexpr char MQTT_R503_COMMAND_LED_BLUE[] = "led_blue";
static constexpr char MQTT_R503_COMMAND_LED_PURPLE[] = "led_purple";
static constexpr char MQTT_R503_COMMAND_LED_OFF[] = "led_off";

static constexpr char MQTT_R503_COMMAND_DELETE[] = "delete_%i";
static constexpr char MQTT_R503_COMMAND_DELETE_ALL[] = "delete_all";
static constexpr char MQTT_R503_COMMAND_ENROLE[] = "enrole_%i";



class r503 : public capability {
	public:
		r503();
		~r503();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool publish();
	private:
		void error_to_logger(uint8 status);
		void error_to_logger(uint8 status, bool verbose);
		void dingdong();
		SoftwareSerial * swSer1;
		Adafruit_Fingerprint * p_finger;
		uint8_t m_state;
		uint8_t m_finger_id;
		uint8_t m_interruptPin;
		uint8_t m_relayPin;
		uint8_t m_DingDongPin;
		uint32_t m_lastsearch;
		uint32_t m_lastdindong;
		bool m_fingerDetected;
		bool m_bellExecuted;
		bool m_blockUntilLift;
		bool m_touch_dingdong;
		bool m_discovery_pub;
		bool m_silent_mode;
};


#endif
