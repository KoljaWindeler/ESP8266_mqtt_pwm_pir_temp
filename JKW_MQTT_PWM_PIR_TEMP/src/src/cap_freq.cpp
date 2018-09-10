#include <cap_freq.h>

// simply the constructor
freq::freq(){
	sprintf((char*)key,"FRE");
};

// simply the destructor
freq::~freq(){
	logger.println(TOPIC_GENERIC_INFO, F("freq deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool freq::parse(uint8_t* config){
	for (uint8_t i = 0; i <= 16; i++) {
		// output non inverted
		//FRE_s_4 == freq counter, seconds , pin 4
		sprintf(m_msg_buffer, "%s_s_%i",get_key(), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin = i;
			m_update = 1; // report as counts per 1 sec
			return true;
		};
		sprintf(m_msg_buffer, "%s_m_%i",get_key(), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin = i;
			m_update = 60; // report as counts per 1 min
			return true;
		};
		sprintf(m_msg_buffer, "%s_h_%i",get_key(), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin = i;
			m_update = 3600; // report as counts per 1 hour
			return true;
		};
	}
	return false;
}

// the will be requested to check if the key is in the config strim
uint8_t* freq::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool freq::init(){
	logger.print(TOPIC_GENERIC_INFO, F("freq counter init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"GPIO %i",m_pin);
	logger.pln(m_msg_buffer);
	m_counter.set(0); // set counter to 0
	pinMode(m_pin, INPUT);
	digitalWrite(m_pin, HIGH); // activate pull up
	m_pin_state = digitalRead(m_pin);
	m_last_update = millis();
	return true;
}

// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t freq::count_intervall_update(){
	// we hi-jack this method to execute some code once per minute
	return 1;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool freq::loop(){
	bool state = digitalRead(m_pin);
	if(m_pin_state != state){ // only update if differen
		m_pin_state = state;
		if(m_pin_state){ // only increment if high
			m_counter.set(m_counter.get_value()+1);
		}
	}
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool freq::intervall_update(uint8_t slot){
	// we've one update so this happens every minute
	// e.g. 8 pulse within a minute, and we want per hour: 8*3600/(60000/1000)=4800 Pph
	sprintf(m_msg_buffer, "%i", (m_counter.get_value()*m_update)/((millis()-m_last_update)/1000));
	logger.print(TOPIC_MQTT_PUBLISH, F("freq "), COLOR_GREEN);
	logger.pln(m_msg_buffer);
	if(network.publish(build_topic(MQTT_FREQ_TOPIC,UNIT_TO_PC), m_msg_buffer)){
		m_counter.set(0);
	}
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool freq::subscribe(){
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool freq::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool freq::publish(){
	return false;
}
