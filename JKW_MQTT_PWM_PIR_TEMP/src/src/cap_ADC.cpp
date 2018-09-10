#include <cap_ADC.h>

ADC::ADC(){};
ADC::~ADC(){
	logger.println(TOPIC_GENERIC_INFO, F("ADC deleted"), COLOR_YELLOW);
};

uint8_t* ADC::get_key(){
	sprintf((char*)key,"ADC");
	return key;
}

bool ADC::init(){
	// output for the analogmeasurement
	pinMode(GPIO_D8, OUTPUT);
	logger.println(TOPIC_GENERIC_INFO, F("ADC init"), COLOR_GREEN);
	return true;
}

bool ADC::loop(){
	return false; // i did nothing
}

uint8_t ADC::count_intervall_update(){
	return 2; // we have 1 value that we want to publish per minute but need a second slot to activate the adc upfront
}

bool ADC::intervall_update(uint8_t slot){
	if(slot==0){
			digitalWrite(GPIO_D8, HIGH);
	} else {
		uint16_t adc = analogRead(A0);
		logger.print(TOPIC_MQTT_PUBLISH, F("adc "), COLOR_GREEN);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", adc);
		logger.pln(m_msg_buffer);
		digitalWrite(GPIO_D8, LOW);
		return network.publish(build_topic(MQTT_ADC_TOPIC,UNIT_TO_PC), m_msg_buffer);
	}
	return false;
}

bool ADC::subscribe(){
	return true; // done
}

bool ADC::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool ADC::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool ADC::publish(){
	return false;
}
