#include <J_GPIO.h>

// simply the constructor
J_GPIO::J_GPIO(){};

// simply the destructor
J_GPIO::~J_GPIO(){
	logger.println(TOPIC_GENERIC_INFO, F("J_GPIO deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool J_GPIO::parse(uint8_t* config){
	bool f=false;
	for(uint8_t i=0; i<=16; i++){
		sprintf(m_msg_buffer,"G%i",i);
		if(cap.parse(config,(uint8_t*)m_msg_buffer)){
			m_pin[i]=true; // mark pin as "in use"
			f = true;
		};
	}
	return f;
}


uint8_t* J_GPIO::get_key(){
	return 0;
}

// will be callen if the m_pin is part of the config
bool J_GPIO::init(){
	for(uint8_t i=0; i<=16; i++){
		if(m_pin[i]){
			digitalWrite(i,LOW);
			pinMode(i, OUTPUT);
			m_invert[i]=false;
			sprintf(m_msg_buffer,"J_GPIO G%i init",i);
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		}
	}
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t J_GPIO::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool J_GPIO::loop(){
	for(uint8_t i=0; i<=16; i++){
		if(m_pin[i]){
			if(millis()>=m_next_action[i] && m_next_action[i]!=0){
				set_output(i,0);
				m_next_action[i]=0;
			}
		}
	}
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool J_GPIO::intervall_update(uint8_t slot){
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool J_GPIO::subscribe(){
	for(uint8_t i=0; i<=16; i++){
		if(m_pin[i]){
			// set J_GPIO state
			sprintf(m_msg_buffer,MQTT_J_GPIO_OUTPUT_TOPIC,i);
			network.subscribe(build_topic(m_msg_buffer,PC_TO_UNIT)); // simple rainbow  topic
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer,PC_TO_UNIT), COLOR_GREEN);

			// change J_GPIO state
			sprintf(m_msg_buffer,MQTT_J_GPIO_TOGGLE_TOPIC,i);
			network.subscribe(build_topic(m_msg_buffer,PC_TO_UNIT)); // simple rainbow  topic
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer,PC_TO_UNIT), COLOR_GREEN);

			// pulse the output
			sprintf(m_msg_buffer,MQTT_J_GPIO_PULSE_TOPIC,i);
			network.subscribe(build_topic(m_msg_buffer,PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer,PC_TO_UNIT), COLOR_GREEN);

			// invert setting
			sprintf(m_msg_buffer,MQTT_J_GPIO_INVERT_TOPIC,i);
			network.subscribe(build_topic(m_msg_buffer,PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer,PC_TO_UNIT), COLOR_GREEN);
		}
	}
	return true;
}

// drive pin
void J_GPIO::set_output(uint8_t pin, uint8_t state){
	if(pin>16){
		return;
	}
	if (state) {
		m_state[pin].set(1);
		sprintf(m_msg_buffer,"Set J_GPIO %i: ON",pin);
	} else {
		m_state[pin].set(0);
		sprintf(m_msg_buffer,"Set J_GPIO %i: OFF",pin);
	}
	logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_PURPLE);
	bool output_state = m_state[pin].get_value();
	if(m_invert[pin]){
		output_state = !output_state;
	}
	digitalWrite(pin,output_state);
};


// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool J_GPIO::receive(uint8_t* p_topic, uint8_t* p_payload){
	for(uint8_t i=0; i<=16; i++){
		if(m_pin[i]){
			// set pin once
			sprintf(m_msg_buffer,MQTT_J_GPIO_OUTPUT_TOPIC,i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer,PC_TO_UNIT))) { // on / off with dimming
				// change OUTPUT
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					set_output(i,1);
				} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
					set_output(i,0);
				}
				return true;
			}

			// change pin
			sprintf(m_msg_buffer,MQTT_J_GPIO_TOGGLE_TOPIC,i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer,PC_TO_UNIT))) { // on / off with dimming
				set_output(i,!m_state[i].get_value());
				return true;
			}

			// pulse output
			sprintf(m_msg_buffer,MQTT_J_GPIO_PULSE_TOPIC,i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer,PC_TO_UNIT))) { // on / off with dimming
				set_output(i,1);
				uint16_t pulse_length = atoi((char*)p_payload);
				m_next_action[i] = millis()+pulse_length;
				// do something
				return true;
			}

			// invert settings
			sprintf(m_msg_buffer,MQTT_J_GPIO_INVERT_TOPIC,i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer,PC_TO_UNIT))) { // on / off with dimming
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					sprintf(m_msg_buffer,"Set J_GPIO %i: inverted",i);
					m_invert[i] = true;
				} else {
					sprintf(m_msg_buffer,"Set J_GPIO %i: non-inverted",i);
					m_invert[i] = false;
				}
				logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_GREEN);
				// do something
				return true;
			}
		}
	}

	// not for me
	return false;
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool J_GPIO::publish(){
	bool ret = false;
	for(uint8_t i=0; i<=16; i++){
		if(m_pin[i]){
			if(m_state[i].get_outdated()){
				sprintf(m_msg_buffer,"J_GPIO %i state ",i);
				logger.print(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);

				sprintf(m_msg_buffer,MQTT_J_GPIO_OUTPUT_TOPIC,i);
				if (m_state[i].get_value()) {
					logger.pln((char*)STATE_ON);
					ret = network.publish(build_topic(m_msg_buffer,UNIT_TO_PC), (char*)STATE_ON);
				} else {
					logger.pln((char*)STATE_OFF);
					ret = network.publish(build_topic(m_msg_buffer,UNIT_TO_PC), (char*)STATE_OFF);
				}
				if (ret) {
					m_state[i].outdated(false);
				}
				return ret; // one at the time
			}
		}
	}
}
