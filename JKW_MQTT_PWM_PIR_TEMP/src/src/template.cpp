#ifdef NEVER_COMPILE_ME
#include <template.h>

// simply the constructor
template::template(){};

// simply the destructor
template::~template(){
	logger.println(TOPIC_GENERIC_INFO, F("template deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool template::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* template::get_key(){
	return (uint8_t*)"TMP";
}

// will be callen if the key is part of the config
bool template::init(){
	logger.println(TOPIC_GENERIC_INFO, F("TMP init"), COLOR_GREEN);
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
//uint8_t template::count_intervall_update(){
//	return 0;
//}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
//bool template::loop(){
//	return false; // i did nothing
//}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
//bool template::intervall_update(uint8_t slot){
//	if(slot%count_intervall_update()==0){
//		logger.print(TOPIC_MQTT_PUBLISH, F(""));
//		dtostrf(template.getSomething(), 3, 2, m_msg_buffer);
//		logger.p(F("Template "));
//		logger.pln(m_msg_buffer);
//		return network.publish(build_topic(MQTT_TEMPLATE_TOPIC,UNIT_TO_PC), m_msg_buffer);
//	}
//	return false;
//}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool template::subscribe(){
//	network.subscribe(build_topic(MQTT_TEMPLATE_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_TEMPLATE_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool template::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_TEMPLATE_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received template command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
//bool template::publish(){
//	return false;
//}
#endif
