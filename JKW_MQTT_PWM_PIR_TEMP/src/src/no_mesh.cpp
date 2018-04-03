#include <no_mesh.h>

// simply the constructor
no_mesh::no_mesh(){};

// simply the destructor
no_mesh::~no_mesh(){};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool no_mesh::parse(uint8_t* config){
	if(cap.parse(config,(uint8_t*)"M0")){ // MESH_MODE_FULL_MESH
		logger.println(TOPIC_GENERIC_INFO, F("Full Mesh enabled"), COLOR_GREEN);
		network.setMeshMode(MESH_MODE_FULL_MESH);
	} else if(cap.parse(config,(uint8_t*)"M1")){ // MESH_MODE_HOST_ONLY
		network.setMeshMode(MESH_MODE_HOST_ONLY);
		logger.println(TOPIC_GENERIC_INFO, F("Mesh host mode enabled"), COLOR_YELLOW);
	} else if(cap.parse(config,(uint8_t*)"M2")){ // MESH_MODE_CLIENT_ONLY
		network.setMeshMode(MESH_MODE_CLIENT_ONLY);
		logger.println(TOPIC_GENERIC_INFO, F("Mesh client mode enabled"), COLOR_YELLOW);
	} else if(cap.parse(config,(uint8_t*)"M3")){ // MESH_MODE_OFF
		network.setMeshMode(MESH_MODE_OFF);
		logger.println(TOPIC_GENERIC_INFO, F("Mesh disabled"), COLOR_GREEN);
	}
	return false; // this will call our destructor
}

// the will be requested to check if the key is in the config strim
uint8_t* no_mesh::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool no_mesh::init(){
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t no_mesh::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool no_mesh::loop(){
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool no_mesh::intervall_update(uint8_t slot){
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool no_mesh::subscribe(){
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool no_mesh::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool no_mesh::publish(){
	return false;
}
