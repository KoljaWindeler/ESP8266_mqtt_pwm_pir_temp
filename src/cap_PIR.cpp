#include <cap_PIR.h>
#ifdef WITH_PIR

///////////////////////////// PARENT ////////////////////////////

PIR::PIR(){
	m_list = NULL;
};

PIR::~PIR(){
	PIR_SENSOR* head=m_list;
	while(head){
		m_list=head->get_next();
		delete head;
		head = m_list;
	}	
	logger.println(TOPIC_GENERIC_INFO, F("PIR deleted"), COLOR_YELLOW);
};

uint8_t* PIR::get_key(){
	return (uint8_t*)"PIR";
}

bool PIR::parse(uint8_t* config){
	uint8_t delay = 0;
	cap.parse_wide(config,"%s%i", (uint8_t*)"PIRDL", 0, 255, &delay, (uint8_t*)"",true);

	if(cap.parse(config,get_key())){
		m_list = new PIR_SENSOR(14,false,delay);
		// set list = 14
	}
	uint8_t m_pin = 255; // uint8_t will overrun to 0 on first call of continuous
	while(cap.parse_wide_continuous(config,(uint8_t*)"PIR",&m_pin)){
		if(m_list){
			m_list->attache_member(new PIR_SENSOR(m_pin,false,delay));
		} else {
			m_list = new PIR_SENSOR(m_pin,false,delay);
		}
	}
	m_pin = 255; // uint8_t will overrun to 0 on first call of continuous
	while(cap.parse_wide_continuous(config,(uint8_t*)"PIRN",&m_pin)){
		if(m_list){
			m_list->attache_member(new PIR_SENSOR(m_pin,true,delay));
		} else {
			m_list = new PIR_SENSOR(m_pin,true,delay);
		}
	}
	// for i in 1..14, parse and return list .. 
	return m_list; // NULL = false, else == true
}

void ICACHE_RAM_ATTR fooPIR(){
	if(p_pir){
		// call with pin as argument
		//uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
		// todo
		((PIR*)p_pir)->interrupt();//gpio_status);
	}
}

bool PIR::init(){
	PIR_SENSOR* next=m_list;
	while(next){
		next->init();
		
		sprintf(m_msg_buffer,"%s init, pin config %i",get_key(),next->get_pin());
		logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		
		next=next->get_next();
	}
	return true;
}

void PIR::interrupt(){
	PIR_SENSOR* next=m_list;
	while(next){
		next->interrupt();
		next=next->get_next();
	}
}

bool PIR::publish(){
	PIR_SENSOR* next=m_list;
	while(next!=NULL){
		if(next->publish()){ 
			// break if correct interrupt sensor found
			return true;
		}
		// else continue
		next=next->get_next();
	}
	return false;
}	

// bool PIR::loop(){
// 	return false;
// }
//
// bool PIR::intervall_update(uint8_t slot){
// 	return false;
// }
//
// bool PIR::subscribe(){
// 	return true;
// }
//
// bool PIR::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }


///////////////////////////// PARENT ////////////////////////////
///////////////////////////// SENSOR ////////////////////////////

PIR_SENSOR::PIR_SENSOR(uint8_t pin,boolean invert,uint8_t delay){
	m_pin = pin;
	m_invert = invert;
	m_init_done = false;
	m_discovery_pub = false;
	m_next = NULL;
	m_id = 0;
	m_delay = delay; // * 100 -> 255 = 25500ms = 25.5sec
	m_triggertime = 0;
}

PIR_SENSOR::~PIR_SENSOR(){
	if(m_init_done){
		detachInterrupt(digitalPinToInterrupt(m_pin));
		m_init_done = false;
	}

#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_M_TOPIC,mqtt.dev_short,m_pin); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing PIR config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif
}

PIR_SENSOR* PIR_SENSOR::get_next(){
	return m_next;
}

uint8_t PIR_SENSOR::get_pin(){
	return m_pin;
};

void PIR_SENSOR::attache_member(PIR_SENSOR* new_next){
	if(m_next){
		m_next->attache_member(new_next);
	} else {
		new_next->m_id = m_id+1;
		m_next = new_next;
	}
}

bool PIR_SENSOR::init(){
	
	if(m_invert){ // low active
		pinMode(m_pin, INPUT_PULLUP); // pull up to avoid interrupts without sensor
	} else {
		pinMode(m_pin,INPUT);
	}
	attachInterrupt(digitalPinToInterrupt(m_pin), fooPIR, CHANGE);
	interrupt(); // grab current state
	m_init_done = true;
	return true;
}
	

bool PIR_SENSOR::interrupt(){
	uint8_t state = digitalRead(m_pin);
	if(m_invert){
		state = !state;
	}
	m_state.check_set(state);
	return false;
}

bool PIR_SENSOR::publish(){
	if (m_state.get_outdated()) { 
		if(millis() > m_triggertime){  // check if we're allow to call this again
			// function called to publish the state of the PIR (on/off)
			boolean ret = false;

			logger.print(TOPIC_MQTT_PUBLISH, F("pir state "), COLOR_GREEN);
			logger.p(m_pin);
			sprintf(m_msg_buffer, MQTT_MOTION_TOPIC, m_pin);
			if (m_state.get_value()) {
				logger.pln(F(": motion"));
				ret = network.publish(build_topic(m_msg_buffer,UNIT_TO_PC), (char*)STATE_ON);
			} else {
				logger.pln(F(": no motion"));
				ret = network.publish(build_topic(m_msg_buffer,UNIT_TO_PC), (char*)STATE_OFF);
			}
			if (ret) {
				m_state.outdated(false);
			}
			m_triggertime = millis() + ((uint16_t)m_delay)*100; // set earlies possible recall to now + 100*delay (ms)
			return ret;
		}
	}

#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_M_TOPIC,mqtt.dev_short,m_pin); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_M_MSG)+2*strlen(mqtt.dev_short)+4+1];
			sprintf(m, MQTT_DISCOVERY_M_MSG, mqtt.dev_short,m_pin,mqtt.dev_short,m_pin);
			logger.print(TOPIC_MQTT_PUBLISH, F("PIR discovery "), COLOR_GREEN);
			logger.pln(m_pin);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			m_state.outdated(true); // force update
			return true;
		}
	}
#endif
	return false;
}

#endif