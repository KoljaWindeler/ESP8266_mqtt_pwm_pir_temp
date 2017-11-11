#include <rssi.h>

rssi::rssi(){};
rssi::~rssi(){
	logger.println(TOPIC_GENERIC_INFO, F("RSSI deleted"), COLOR_YELLOW);
};
void rssi::interrupt(){};

uint8_t* rssi::get_key(){
	sprintf((char*)key,"R");
	return key;
}

bool rssi::init(){
	logger.println(TOPIC_GENERIC_INFO, F("RSSI init"), COLOR_GREEN);
}

bool rssi::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool rssi::loop(){
	return false; // i did nothing
}

uint8_t rssi::count_intervall_update(){
	return 1; // we have 1 value that we want to publish per minute
}

bool rssi::intervall_update(uint8_t slot){
	float rssi = wifiManager.getRSSIasQuality((int)WiFi.RSSI());
	//float rssi = WiFi.RSSI();
	logger.print(TOPIC_MQTT_PUBLISH, F("rssi "), COLOR_GREEN);
	// this is needed to avoid reporting the same value over and over
	// home assistant will show us as "not updated for xx Minutes" if the RSSSI stays the same
	rssi += ((float) random(10)) / 100;
	dtostrf(rssi, 3, 2, m_msg_buffer);
	Serial.println(m_msg_buffer);
	return client.publish(build_topic(MQTT_RSSI_STATE_TOPIC), m_msg_buffer, true);
}

bool rssi::subscribe(){
	return true;
}

bool rssi::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool rssi::publish(){
	return false;
}
