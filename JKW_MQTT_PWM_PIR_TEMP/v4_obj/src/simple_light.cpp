#include <simple_light.h>

simple_light::simple_light(){};
simple_light::~simple_light(){
	logger.println(TOPIC_GENERIC_INFO, F("Simple light deleted"), COLOR_YELLOW);
};
void simple_light::interrupt(){};

uint8_t* simple_light::get_key(){
	sprintf((char*)key,"SL");
	return key;
}

bool simple_light::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool simple_light::init(){
	pinMode(SIMPLE_LIGHT_PIN, OUTPUT);
	logger.println(TOPIC_GENERIC_INFO, F("Simple light init"), COLOR_GREEN);
	//setState();
}

bool simple_light::loop(){
	return false;
}

uint8_t simple_light::count_intervall_update(){
	return 0; // we have 0 value that we want to publish per minute
}

bool simple_light::intervall_update(uint8_t slot){
	return false;
}

bool simple_light::subscribe(){
	return true;
}

bool simple_light::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool simple_light::publish(){
	if (m_state.get_outdated()) {
		boolean ret = false;
		logger.print(TOPIC_MQTT_PUBLISH, F("simple light state "), COLOR_GREEN);
		if (m_state.get_value()) {
			Serial.println(STATE_ON);
			ret = client.publish(build_topic(MQTT_SIMPLE_LIGHT_STATE_TOPIC), STATE_ON, true);
		} else {
			Serial.println(STATE_OFF);
			ret = client.publish(build_topic(MQTT_SIMPLE_LIGHT_STATE_TOPIC), STATE_OFF, true);
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}
	return false;
}

// function called to adapt the state of the led
void simple_light::setColor(uint8_t r, uint8_t g, uint8_t b){
	m_state.set(r);
	if (r) {
		digitalWrite(SIMPLE_LIGHT_PIN, HIGH);
		logger.println(TOPIC_INFO_SL, F("Simple pin on"));
	} else {
		digitalWrite(SIMPLE_LIGHT_PIN, LOW);
		logger.println(TOPIC_INFO_SL, F("Simple light off"));
	}
}
