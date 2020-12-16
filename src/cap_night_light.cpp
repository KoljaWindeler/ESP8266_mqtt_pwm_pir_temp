#include <cap_night_light.h>
#ifdef WITH_NL
// simply the constructor
night_light::night_light(){};

// simply the destructor
night_light::~night_light(){
	logger.println(TOPIC_GENERIC_INFO, F("Night light deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool night_light::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* night_light::get_key(){
	return (uint8_t*)"NL";
}

// will be callen if the key is part of the config
bool night_light::init(){
	pinMode(PIN_NIGHT_LIGHT,OUTPUT);
	logger.println(TOPIC_GENERIC_INFO, F("Night light init"), COLOR_GREEN);
	return true;
}


// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
// bool night_light::loop(){
// 	return false; // i did nothing
// }

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
// bool night_light::intervall_update(uint8_t slot){
// 	return false;
// }

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool night_light::subscribe(){
	network.subscribe(build_topic(MQTT_NIGHT_LIGHT_TOPIC, strlen(MQTT_NIGHT_LIGHT_TOPIC),PC_TO_UNIT,false)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_NIGHT_LIGHT_TOPIC, strlen(MQTT_NIGHT_LIGHT_TOPIC),PC_TO_UNIT,false), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool night_light::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_NIGHT_LIGHT_TOPIC, strlen(MQTT_NIGHT_LIGHT_TOPIC),PC_TO_UNIT,false))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received night_light command"),COLOR_PURPLE);
		if (!strcmp_P((const char *) p_payload, STATE_ON)) {
			digitalWrite(PIN_NIGHT_LIGHT,LOW);
		} else {
			digitalWrite(PIN_NIGHT_LIGHT,HIGH);
		}
		// do something
		return true;
	}
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
// bool night_light::publish(){
// 	return false;
// }
#endif
