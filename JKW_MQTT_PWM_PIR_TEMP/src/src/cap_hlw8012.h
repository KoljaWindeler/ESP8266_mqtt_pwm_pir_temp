#ifndef hlw8012_h
#define hlw8012_h

#include "main.h"
#include "HLW8012.h"

#define SEL_PIN                         5
#define CF1_PIN                         13
#define CF_PIN                          14

// Check values every 10 seconds
#define UPDATE_TIME                     5000

// Set SEL_PIN to HIGH to sample current
// This is the case for Itead's Sonoff POW, where a
// the SEL_PIN drives a transistor that pulls down
// the SEL pin in the HLW8012 when closed
#define CURRENT_MODE                    HIGH

// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 509000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

static constexpr char MQTT_HLW_CALIBRATE_TOPIC[] = "calibration";
static constexpr char MQTT_HLW_CURRENT_TOPIC[]   = "current";
static constexpr char MQTT_HLW_VOLTAGE_TOPIC[]   = "voltage";
static constexpr char MQTT_HLW_POWER_TOPIC[]     = "power";


class J_hlw8012 : public peripheral {
	public:
		J_hlw8012();
		~J_hlw8012();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool parse(uint8_t* config);
		uint8_t* get_key();
		bool publish();
		HLW8012 m_hlw8012;
	private:
		uint8_t key[4];
};


#endif
