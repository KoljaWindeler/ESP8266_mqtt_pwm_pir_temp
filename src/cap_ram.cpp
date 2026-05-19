#include <cap_ram.h>
#ifdef WITH_RAM

ram::ram(){
	m_discovery_pub = false;
};
ram::~ram(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		undiscover(DOMAIN_SENSOR,MQTT_RAM_TOPIC);
		logger.println(TOPIC_GENERIC_INFO, F("ram deleted"), COLOR_YELLOW);
		m_discovery_pub = false;
	}
#endif
};

uint8_t* ram::get_key(){
	return (uint8_t*)"RAM";
}

bool ram::init(){
	logger.println(TOPIC_GENERIC_INFO, F("ram init"), COLOR_GREEN);
	return true;
}

bool ram::parse(uint8_t* config){
	cap.parse(config,get_key()); // consume key
	return true; // load always
}

// bool ram::loop(){
// 	return false; // i did nothing
// }

uint8_t ram::count_intervall_update(){
	return 1; // we have 1 value that we want to publish per minute
}

bool ram::intervall_update(uint8_t slot){
	uint32_t ram = system_get_free_heap_size();
	//float ram = WiFi.ram();
	logger.print(TOPIC_MQTT_PUBLISH, F("ram "), COLOR_GREEN);
	sprintf(m_msg_buffer,"%u",ram);
	logger.pln(m_msg_buffer);
	return network.publish(build_topic(MQTT_RAM_TOPIC,UNIT_TO_PC), m_msg_buffer);
}

// bool ram::subscribe(){
// 	return true;
// }
//
// bool ram::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
bool ram::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			if(discovery(DOMAIN_SENSOR,MQTT_RAM_TOPIC,UNIT_NONE)){
					m_discovery_pub = true;
			}
			return true;
		}
	}
#endif
	return false; // did nothing
}

#endif
