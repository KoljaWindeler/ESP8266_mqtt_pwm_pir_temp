#include <cap_PIR.h>
#ifdef WITH_PIR

PIR::PIR(){
	init_done = false;
	m_discovery_pub = false;
};

PIR::~PIR(){
	if(init_done){
		detachInterrupt(digitalPinToInterrupt(m_pin));
		init_done = false;
	}

#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_M_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing PIR config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif

	logger.println(TOPIC_GENERIC_INFO, F("PIR deleted"), COLOR_YELLOW);
};

uint8_t* PIR::get_key(){
	return (uint8_t*)"PIR";
}

bool PIR::parse(uint8_t* config){
	if(cap.parse(config,get_key())){
		m_pin = 14;
		return true;
	}
	return cap.parse_wide(config,get_key(),&m_pin);
}

void ICACHE_RAM_ATTR fooPIR(){
	if(p_pir){
		((PIR*)p_pir)->interrupt();
	}
}

bool PIR::init(){
	pinMode(m_pin, INPUT);
	digitalWrite(m_pin, HIGH); // pull up to avoid interrupts without sensor
	attachInterrupt(digitalPinToInterrupt(m_pin), fooPIR, CHANGE);
	init_done = true;
	sprintf(m_msg_buffer,"%s init, pin config %i",get_key(),m_pin);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
	return true;
}

void PIR::interrupt(){
	m_state.check_set(digitalRead(m_pin));
}

// bool PIR::loop(){
// 	return false;
// }
//
// bool PIR::intervall_update(uint8_t slot){
// 	return false;
// }
//
// bool PIR::subscribe(){
// 	return true;
// }
//
// bool PIR::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }


bool PIR::publish(){
	if (m_state.get_outdated()) {
		// function called to publish the state of the PIR (on/off)
		boolean ret = false;

		logger.print(TOPIC_MQTT_PUBLISH, F("pir state "), COLOR_GREEN);
		if (m_state.get_value()) {
			logger.pln(F("motion"));
			ret = network.publish(build_topic(MQTT_MOTION_TOPIC,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			logger.pln(F("no motion"));
			ret = network.publish(build_topic(MQTT_MOTION_TOPIC,UNIT_TO_PC), (char*)STATE_OFF);
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}

#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_M_TOPIC); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_M_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_M_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("PIR discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif


	return false;
}

#endif
