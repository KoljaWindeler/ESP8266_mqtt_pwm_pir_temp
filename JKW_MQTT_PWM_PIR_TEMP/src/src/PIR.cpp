#include <PIR.h>

PIR::PIR(uint8_t* k,uint8_t pin){
	m_pin = pin;
	sprintf((char*)key,(char*)k);
};

PIR::~PIR(){
	detachInterrupt(digitalPinToInterrupt(m_pin));
	uint8_t buffer[15];
	sprintf((char*)buffer,"%s deleted",get_key());
	logger.println(TOPIC_GENERIC_INFO, (char*)buffer, COLOR_YELLOW);
};


uint8_t* PIR::get_key(){
	return key;
}

bool PIR::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

void fooPIR(){
	if(p_pir){
		((PIR*)p_pir)->interrupt();
	} else if(p_pir2){
		((PIR*)p_pir2)->interrupt();
	}
}

bool PIR::init(){
	pinMode(m_pin, INPUT);
	digitalWrite(m_pin, HIGH); // pull up to avoid interrupts without sensor
	attachInterrupt(digitalPinToInterrupt(m_pin), fooPIR, CHANGE);
	sprintf(m_msg_buffer,"%s init, pin config %i",get_key(),m_pin);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
}

void PIR::interrupt(){
	m_state.check_set(digitalRead(m_pin));
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
