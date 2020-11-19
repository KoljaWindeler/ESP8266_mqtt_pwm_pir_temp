#include <cap_rssi.h>
#ifdef WITH_RSSI

rssi::rssi(){};
rssi::~rssi(){
	logger.println(TOPIC_GENERIC_INFO, F("RSSI deleted"), COLOR_YELLOW);
};

uint8_t* rssi::get_key(){
	return (uint8_t*)"R";
}

bool rssi::init(){
	logger.println(TOPIC_GENERIC_INFO, F("RSSI init"), COLOR_GREEN);
	return true;
}

bool rssi::parse(uint8_t* config){
	cap.parse(config,get_key()); // consume key
	return true; // load always
}

// bool rssi::loop(){
// 	return false; // i did nothing
// }

uint8_t rssi::count_intervall_update(){
	return 1; // we have 1 value that we want to publish per minute
}

bool rssi::intervall_update(uint8_t slot){
	float rssi = wifiManager.getRSSIasQuality((int)WiFi.RSSI());
	//float rssi = WiFi.RSSI();
	logger.print(TOPIC_MQTT_PUBLISH, F("rssi "), COLOR_GREEN);
	// this is needed to avoid reporting the same value over and over
	// home assistant will show us as "not updated for xx Minutes" if the RSSSI stays the same
	rssi -= ((float) random(10)) / 10; // 0.0 to 0.9 less 
	dtostrf(rssi, 2, 1, m_msg_buffer);
	logger.pln(m_msg_buffer);
	return network.publish(build_topic(MQTT_RSSI_TOPIC,UNIT_TO_PC), m_msg_buffer);
}

// bool rssi::subscribe(){
// 	return true;
// }
//
// bool rssi::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
// bool rssi::publish(){
// 	return false;
// }

#endif
