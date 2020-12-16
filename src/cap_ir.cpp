#ifdef WITH_IR
#include <cap_ir.h>


// simply the constructor
ir::ir(){};

// simply the destructor
ir::~ir(){
	logger.println(TOPIC_GENERIC_INFO, F("IR deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool ir::parse(uint8_t* config){
	return cap.parse_wide(config,get_key(),&m_pin);
}

// the will be requested to check if the key is in the config strim
uint8_t* ir::get_key(){
	return (uint8_t*)"IR";
}

// will be callen if the key is part of the config
bool ir::init(){
	pMyReceiver = new IRrecv(m_pin); // gpio 5 -> d1
	pResults = new decode_results;
	pMyReceiver->enableIRIn();
	logger.print(TOPIC_GENERIC_INFO, F("IR init, GPIO "), COLOR_GREEN);
	Serial.println(m_pin);
	m_discovery_pub  = false;
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
//uint8_t ir::count_intervall_update(){
//	return 0;
//}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool ir::loop(){
	if (pMyReceiver->decode(pResults)) {
		if(0xFFFFFFFFFFFFFFFF!=pResults->value){
			m_state.set(pResults->value);
			//Serial.println(typeToString(pResults->decode_type,0));
		}
		pMyReceiver->resume();  // Receive the next value
  }
	return false; // i did nothing
}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
//bool ir::intervall_update(uint8_t slot){
//	if(slot%count_intervall_update()==0){
//		logger.print(TOPIC_MQTT_PUBLISH, F(""));
//		dtostrf(ir.getSomething(), 3, 2, m_msg_buffer);
//		logger.p(F("ir "));
//		logger.pln(m_msg_buffer);
//		return network.publish(build_topic(MQTT_ir_TOPIC,UNIT_TO_PC), m_msg_buffer, true);
//	}
//	return false;
//}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool ir::subscribe(){
//	network.subscribe(build_topic(MQTT_ir_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_ir_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool ir::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_ir_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received ir command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool ir::publish(){
	if (m_state.get_outdated()) {
		uint64_t t=m_state.get_value();
		sprintf(m_msg_buffer, "IR recv %04X%04X%04X%04X",((uint16_t)((t>>48)&0xffff)),((uint16_t)((t>>32)&0xffff)),((uint16_t)((t>>16)&0xffff)),((uint16_t)(t&0xffff)));
		logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
		if(network.publish(build_topic(MQTT_IR_TOPIC, UNIT_TO_PC), (char *) m_msg_buffer+9)){
			m_state.outdated(false);
			return true;
		}
	}

	#ifdef WITH_DISCOVERY
		if(!m_discovery_pub){
			if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
				char* t = discovery_topic_bake(MQTT_DISCOVERY_IR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
				char* m = new char[strlen(MQTT_DISCOVERY_IR_MSG)+2*strlen(mqtt.dev_short)];
				sprintf(m, MQTT_DISCOVERY_IR_MSG, mqtt.dev_short, mqtt.dev_short);
				logger.println(TOPIC_MQTT_PUBLISH, F("IR discovery"), COLOR_GREEN);
				m_discovery_pub = network.publish(t,m);
				delete[] m;
				delete[] t;
				return true;
			}
		}
	#endif
	return false;
}
#endif
