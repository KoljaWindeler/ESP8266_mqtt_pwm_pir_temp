#include <cap_simple_light.h>

simple_light::simple_light(){};
simple_light::~simple_light(){
	logger.println(TOPIC_GENERIC_INFO, F("Simple light deleted"), COLOR_YELLOW);
};

uint8_t* simple_light::get_key(){
	sprintf((char*)key,"SL");
	return key;
}

bool simple_light::parse(uint8_t* config){
	if(cap.parse(config,get_key(),(uint8_t*)"LIG")){
		m_pin = SIMPLE_LIGHT_PIN;
		return true;
	}
	// check for all pins with dedicated string
	else if(cap.parse_wide(config, get_key(),&m_pin, (uint8_t*)"LIG")){
		return true;
	}

	return false;
}

bool simple_light::init(){
	pinMode(m_pin, OUTPUT);
	logger.print(TOPIC_GENERIC_INFO, F("Simple light init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"GPIO %i",m_pin);
	logger.pln(m_msg_buffer);
	//setState();
}

bool simple_light::loop(){
	return false;
}

uint8_t simple_light::count_intervall_update(){
	return 0; // we have 0 value that we want to publish per minute
}

bool simple_light::intervall_update(uint8_t slot){
	return false;
}

bool simple_light::subscribe(){
	return true;
}

bool simple_light::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool simple_light::publish(){	return false;}

// function called to adapt the state of the led
void simple_light::setColor(uint8_t r, uint8_t g, uint8_t b){
	m_state.set(r);
	if (r) {
		digitalWrite(m_pin, HIGH);
		logger.println(TOPIC_INFO_SL, F("Simple pin on"), COLOR_PURPLE);
	} else {
		digitalWrite(m_pin, LOW);
		logger.println(TOPIC_INFO_SL, F("Simple light off"), COLOR_PURPLE);
	}
}
