#include <dimmer.h>

dimmer::dimmer(){};
dimmer::~dimmer(){};

uint8_t dimmer::get_key(){
	return ' ';
}

bool dimmer::init(){
}


bool dimmer::loop(){
	return false; // i did nothing
}

bool dimmer::intervall_update(){
	return false;
}

bool dimmer::subscribe(){
	return true;
}

bool dimmer::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool dimmer::publish(){
	return true;
}
