#include <cap_ADC.h>
#ifdef WITH_ADC

ADC::ADC(){
	m_discovery_pub = false;
};
ADC::~ADC(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_ADC_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ADC config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("ADC deleted"), COLOR_YELLOW);
};

uint8_t* ADC::get_key(){
	return (uint8_t*)"ADC";
}

bool ADC::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool ADC::init(){
	// output for the analogmeasurement
	//pinMode(GPIO_D8, OUTPUT);
	logger.println(TOPIC_GENERIC_INFO, F("ADC init"), COLOR_GREEN);
	return true;
}


uint8_t ADC::count_intervall_update(){
	return 2; // we have 1 value that we want to publish per minute but need a second slot to activate the adc upfront
}

bool ADC::intervall_update(uint8_t slot){
	//if(slot==0){
	//		digitalWrite(GPIO_D8, HIGH);
	//} else {
		uint16_t adc = analogRead(A0);
		logger.print(TOPIC_MQTT_PUBLISH, F("adc "), COLOR_GREEN);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", adc);
		logger.pln(m_msg_buffer);
	//	digitalWrite(GPIO_D8, LOW);
		return network.publish(build_topic(MQTT_ADC_TOPIC,UNIT_TO_PC), m_msg_buffer);
	//}
	return false;
}

//bool ADC::loop(){
//	return false; // i did nothing
//}

//bool ADC::subscribe(){
//	return true; // done
//}

//bool ADC::receive(uint8_t* p_topic, uint8_t* p_payload){
//	return false; // not for me
//}

bool ADC::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			if(discovery(DOMAIN_SENSOR,MQTT_ADC_TOPIC,UNIT_NONE)){
				logger.println(TOPIC_MQTT_PUBLISH, F("ADC discovery"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
			}
			return m_discovery_pub;
		}
	}
#endif
	return false;
}

#endif
