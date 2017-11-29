#include <PIR.h>

PIR::PIR(){};
PIR::~PIR(){
	logger.println(TOPIC_GENERIC_INFO, F("PIR deleted"), COLOR_YELLOW);
};


uint8_t* PIR::get_key(){
	sprintf((char*)key,"PIR");
	return key;
}

bool PIR::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

void fooPIR(){
	if(p_pir){
		p_pir->interrupt();
	}
}

bool PIR::init(){
	pinMode(PIR_PIN, INPUT);
	digitalWrite(PIR_PIN, HIGH); // pull up to avoid interrupts without sensor
	attachInterrupt(digitalPinToInterrupt(PIR_PIN), fooPIR, CHANGE);
	logger.println(TOPIC_GENERIC_INFO, F("PIR init"), COLOR_GREEN);
}

void PIR::interrupt(){
	m_state.check_set(digitalRead(PIR_PIN));
}

bool PIR::loop(){
	return false;
}

uint8_t PIR::count_intervall_update(){
	return 0; // we have 0 value that we want to publish per minute
}

bool PIR::intervall_update(uint8_t slot){
	return false;
}

bool PIR::subscribe(){
	return true;
}

bool PIR::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool PIR::publish(){
	if (m_state.get_outdated()) {
		// function called to publish the state of the PIR (on/off)
		boolean ret = false;

		logger.print(TOPIC_MQTT_PUBLISH, F("pir state "), COLOR_GREEN);
		if (m_state.get_value()) {
			logger.pln(F("motion"));
			ret = client.publish(build_topic(MQTT_MOTION_TOPIC,UNIT_TO_PC), STATE_ON, true);
		} else {
			logger.pln(F("no motion"));
			ret = client.publish(build_topic(MQTT_MOTION_TOPIC,UNIT_TO_PC), STATE_OFF, true);
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}
	return false;
}
