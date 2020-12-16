#include <cap_hx711.h>
#ifdef WITH_HX711
// simply the constructor
hx711::hx711(){
	m_discovery_pub = false;
};

// simply the destructor
hx711::~hx711(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_HX711_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing HX711 config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("hx711 deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool hx711::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* hx711::get_key(){
	return (uint8_t*)"HX711";
}

// will be callen if the key is part of the config
bool hx711::init(){
	p_hx = new HX711_c();
	p_hx->begin(14,5);
	p_hx->power_up();
	logger.println(TOPIC_GENERIC_INFO, F("HX711 init"), COLOR_GREEN);
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
uint8_t hx711::count_intervall_update(){
	return 10; // 1;
}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
//bool hx711::loop(){
//	return false; // i did nothing
//}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool hx711::intervall_update(uint8_t slot){
	logger.print(TOPIC_MQTT_PUBLISH, F(""));
	sprintf(m_msg_buffer, "%i", p_hx->get_value(10));
	logger.p(F("hx711 "));
	logger.pln(m_msg_buffer);
	return network.publish(build_topic(MQTT_HX711_TOPIC,UNIT_TO_PC), m_msg_buffer);
}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool hx711::subscribe(){
	network.subscribe(build_topic(MQTT_HX711_TARE_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_HX711_TARE_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool hx711::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_HX711_TARE_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received hx711 tare command"),COLOR_PURPLE);
		p_hx->set_offset(atol((const char*)p_payload));
		return true;
	}
	return false; // not for me
}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool hx711::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_HX711_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_HX711_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_HX711_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("HX711 discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
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