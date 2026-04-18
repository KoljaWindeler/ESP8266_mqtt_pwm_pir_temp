#include <cap_DHT22.h>
#ifdef WITH_DHT22

J_DHT22::J_DHT22(){
	m_discovery_pub = false;
};

J_DHT22::~J_DHT22(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		undiscover(DOMAIN_SENSOR,MQTT_TEMPARATURE_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing DHT temp config "), COLOR_YELLOW);
		undiscover(DOMAIN_SENSOR,MQTT_HUMIDITY_TOPIC);
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing DHT humidity config "), COLOR_YELLOW);
		m_discovery_pub = false;
	}
#endif

	logger.println(TOPIC_GENERIC_INFO, F("DHT deleted"), COLOR_YELLOW);
};

uint8_t* J_DHT22::get_key(){
	return (uint8_t*)"DHT";
}

bool J_DHT22::init(){
	// dht
	p_dht = new DHT(m_pin, DHT22);        // J_DHT22
	p_dht->begin();
	logger.println(TOPIC_GENERIC_INFO, F("DHT init"), COLOR_GREEN);
	return true;
}

uint8_t J_DHT22::count_intervall_update(){
	return 2; // we have 2 values; temp + hum that we want to publish per minute
}

bool J_DHT22::parse(uint8_t* config){
	if(cap.parse(config,get_key())){
		m_pin = DHT_PIN;
		return true;
	}
	return cap.parse_wide(config,get_key(),&m_pin);
}


bool J_DHT22::intervall_update(uint8_t slot){
	if(slot==0){
		float temp = p_dht->readTemperature();
		if (temp > TEMP_MAX || temp < (-1 * TEMP_MAX) || isnan(temp)) {
			logger.print(TOPIC_GENERIC_INFO, F("no publish temp, "), COLOR_YELLOW);
			if (isnan(temp)) {
				logger.pln(F("nan"));
			} else {
				logger.p(temp);
				logger.pln(F(" >TEMP_MAX"));
			}
			return false;
		}

		dtostrf(temp, 3, 2, m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("DHT temp "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_TEMPARATURE_TOPIC,UNIT_TO_PC), m_msg_buffer);
	}
	else if(slot==1){
		float hum = p_dht->readHumidity();
		if (isnan(hum)) {
			logger.println(TOPIC_GENERIC_INFO, F("no publish humidity"), COLOR_YELLOW);
			return false;
		}
		logger.print(TOPIC_MQTT_PUBLISH, F("humidity "), COLOR_GREEN);
		dtostrf(hum, 3, 2, m_msg_buffer);
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_HUMIDITY_TOPIC,UNIT_TO_PC), m_msg_buffer);
	}
	return false;
}


// bool J_DHT22::loop(){
// 	return false; // i did nothing
// }
// bool J_DHT22::subscribe(){
// 	return true;
// }
//
// bool J_DHT22::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
bool J_DHT22::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			if(discovery(DOMAIN_SENSOR,MQTT_TEMPARATURE_TOPIC,UNIT_DEG_C)){
					m_discovery_pub = true;
					logger.println(TOPIC_MQTT_PUBLISH, F("DHT temp discovery"), COLOR_GREEN);
			}
			if(discovery(DOMAIN_SENSOR,MQTT_HUMIDITY_TOPIC,UNIT_PERC)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("DHT humidity discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			return true;
		}
	}
#endif
	return false; // did nothing
}
#endif
