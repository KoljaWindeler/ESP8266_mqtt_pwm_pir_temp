#include <uptime.h>

// simply the constructor
uptime::uptime(){
	sprintf((char*)key,"UT");
};

// simply the destructor
uptime::~uptime(){
	logger.println(TOPIC_GENERIC_INFO, F("uptime deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool uptime::parse(uint8_t* config){
	for (uint8_t i = 0; i <= 16; i++) {
		// output non inverted
		//UTH5 == uptime counter, high active, pin 5
		sprintf(m_msg_buffer, "%sP%i",get_key(), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin = i;
			m_active_high = true;
			return true;
		};
		sprintf(m_msg_buffer, "%sN%i",get_key(), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin = i;
			m_active_high = false; // active low config
			return true;
		};
	}
	return false;
}

// the will be requested to check if the key is in the config strim
uint8_t* uptime::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool uptime::init(){
	logger.print(TOPIC_GENERIC_INFO, F("Uptime counter init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"GPIO %i",m_pin);
	logger.pln(m_msg_buffer);
	m_counter.set(0); // set counter to 0
	if(m_active_high){
		pinMode(m_pin, INPUT);
		digitalWrite(m_pin, LOW); // no pull up
	} else {
		pinMode(m_pin, INPUT);
		digitalWrite(m_pin, HIGH); // activate pull up
	}
	return true;
}

// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t uptime::count_intervall_update(){
	// we hi-jack this method to execute some code once per minute
	return 1;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool uptime::loop(){
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool uptime::intervall_update(uint8_t slot){
	if(digitalRead(m_pin) == m_active_high){ // pin is active
		m_counter.set(m_counter.get_value()+1); // update eveny minute if the pin is active
	}
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool uptime::subscribe(){
	network.subscribe(build_topic(MQTT_UPTIME_RESET_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_UPTIME_RESET_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool uptime::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_UPTIME_RESET_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received uptime reeset command"),COLOR_PURPLE);
		m_counter.set(0); // reset
		return true;
	}
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool uptime::publish(){
	if(m_counter.get_outdated()){
		sprintf(m_msg_buffer, "%i", m_counter.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Uptime "), COLOR_GREEN);
		logger.pln(m_msg_buffer);

		if(network.publish(build_topic(MQTT_UPTIME_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_counter.outdated(false);
		}
		return true;
	}
	return false;
}
