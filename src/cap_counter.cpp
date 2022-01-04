#include <cap_counter.h>
#ifdef WITH_COUNTER

// simply the constructor
counter::counter(){
    m_discovery_pub = false;
};

// simply the destructor
counter::~counter(){
    #ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_COUNTER_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing counter config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("counter deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool counter::parse(uint8_t* config){
	if(cap.parse_wide(config,get_key(),&m_pin)){
		uint8_t ghost_sec = 0;
		cap.parse_wide(config,"%s%i", (uint8_t*)"GHOST", 0, 255, &ghost_sec, (uint8_t*)""); // GHOST1 = 1000ms GHOST time here .. 
		m_ghost = ghost_sec * 1000; // convert to ms
		return true;
	}
    return false;
}

// the will be requested to check if the key is in the config strim
uint8_t* counter::get_key(){
	return (uint8_t*)"COUNTER";
}

// will be callen if the key is part of the config
bool counter::init(){
    pinMode(m_pin,INPUT_PULLUP);
    m_state.set(0);
    m_state.outdated(false);
    m_pin_was = false;
	m_last_edge = 0;
    sprintf(m_msg_buffer,"Counter init on gpio %i",m_pin);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
	return true;
}

// override-methode, only implement when needed
// e.g. the light class needs a provider to actually do something, so it implements this
//bool play::reg_provider(peripheral * p, uint8_t* t){
//	return false;
//}

// override-methode, only implement when needed
// return the key of any component that you depend on, e.g. "PWM" depends on "LIG"
//uint8_t* play::get_dep(){
//	return (uint8_t*)"";
//}

// override-methode, only implement when needed
// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
//uint8_t counter::count_intervall_update(){
//	return 0;
//}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool counter::loop(){
    if(m_pin_was == HIGH){ 
        // pin was high last time .. 
        if(digitalRead(m_pin) == LOW){
            // pin just transitioned from high to low, e.g. switched closed 
            m_pin_was = LOW;
			if(millis()-m_last_edge>m_ghost){
            	m_state.set(m_state.get_value()+1);
			} else {
				logger.println(TOPIC_MQTT, F("Double Pulse avoided"),COLOR_RED);
			}
			m_last_edge = millis();
        }
    } else {
        // pin was low last time, wait for a transition to go high
        if(digitalRead(m_pin) == HIGH){
            // pin transitioned to high, toggle again
            m_pin_was = HIGH;
        }
    }
	return false; // i did nothing
}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
//bool counter::intervall_update(uint8_t slot){
//	if(slot%count_intervall_update()==0){
//		logger.print(TOPIC_MQTT_PUBLISH, F(""));
//		dtostrf(counter.getSomething(), 3, 2, m_msg_buffer);
//		logger.p(F("counter "));
//		logger.pln(m_msg_buffer);
//		return network.publish(build_topic(MQTT_counter_TOPIC,UNIT_TO_PC), m_msg_buffer);
//	}
//	return false;
//}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool counter::subscribe(){
	network.subscribe(build_topic(MQTT_COUNTER_TOPIC,UNIT_TO_PC)); // Attention, UNIT TO PC! so we describe to dev4/r/counter ... /r/ ... RRR 
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_COUNTER_TOPIC,UNIT_TO_PC), COLOR_GREEN);
	return true;
}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool counter::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_COUNTER_TOPIC,UNIT_TO_PC))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received counter command"),COLOR_PURPLE);
		m_state.check_set(atol((const char*)p_payload));
        if(m_state.get_outdated()){
            logger.println(TOPIC_MQTT, F("Received updated counter value"),COLOR_GREEN);
        }
		network.unsubscribe(build_topic(MQTT_COUNTER_TOPIC,UNIT_TO_PC)); // Attention, UNIT TO PC! so we describe to dev4/r/counter ... /r/ ... RRR 
		return true;
	}
	return false; // not for me
}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool counter::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_COUNTER_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_COUNTER_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_COUNTER_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("Counter discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
		}
	}
#endif
    if (m_state.get_outdated()) {
        logger.print(TOPIC_MQTT_PUBLISH, F("Counter update: "), COLOR_GREEN);
        ltoa(m_state.get_value(),m_msg_buffer,10);
        logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_COUNTER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
            m_state.outdated(false);
            return true;
        }
    }
	return false;
}
#endif
