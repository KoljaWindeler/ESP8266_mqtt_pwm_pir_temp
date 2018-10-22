#include <cap_DHT22.h>
#ifdef WITH_DHT22

J_DHT22::J_DHT22(){};
J_DHT22::~J_DHT22(){
	logger.println(TOPIC_GENERIC_INFO, F("DHT deleted"), COLOR_YELLOW);
};

uint8_t* J_DHT22::get_key(){
	return (uint8_t*)"DHT";
}

bool J_DHT22::init(){
	// dht
	p_dht = new DHT(DHT_PIN, DHT22);        // J_DHT22
	p_dht->begin();
	logger.println(TOPIC_GENERIC_INFO, F("DHT init"), COLOR_GREEN);
	return true;
}

uint8_t J_DHT22::count_intervall_update(){
	return 2; // we have 2 values; temp + hum that we want to publish per minute
}

bool J_DHT22::parse(uint8_t* config){
	return cap.parse(config,get_key());
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
			logger.println(TOPIC_GENERIC_INFO, F("no publish humidiy"), COLOR_YELLOW);
			return false;
		}
		logger.print(TOPIC_MQTT_PUBLISH, F("humidiy "), COLOR_GREEN);
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
// bool J_DHT22::publish(){
// 	return false; // did nothing
// }
#endif
