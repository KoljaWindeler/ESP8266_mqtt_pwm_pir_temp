#include <cap_remote_simple_light.h>
#ifdef WITH_SL

remote_simple_light::remote_simple_light(){};

remote_simple_light::~remote_simple_light(){
	// no discovery here
	// this is not a primary light provider, more a relay
	logger.println(TOPIC_GENERIC_INFO, F("Remote Simple light deleted"), COLOR_YELLOW);
};

uint8_t* remote_simple_light::get_key(){
	return (uint8_t*)"RSL";
}

uint8_t* remote_simple_light::get_dep(){
	return (uint8_t*)"LIG";
}

bool remote_simple_light::parse(uint8_t* config){
	// check for all pins with dedicated string
	if(cap.parse_wide(config, "%s%i", get_key(), 0, 255, &m_dev_id, get_dep(), true)){
		return true;
	}
	return false;
}

bool remote_simple_light::init(){
	logger.print(TOPIC_GENERIC_INFO, F("Remote simple light init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"Dev id %i",m_dev_id);
	logger.pln(m_msg_buffer);
	// set state? TODO
	m_state.set(0);
	return true;
}

// bool remote_simple_light::loop(){
// 	return false;
// }
//
// bool remote_simple_light::intervall_update(uint8_t slot){
// 	return false;
// }
//
// bool remote_simple_light::subscribe(){
// 	return true;
// }
//
// bool remote_simple_light::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
bool remote_simple_light::publish(){
	return false;
}

uint8_t remote_simple_light::get_modes(){
	return 0; // only simple on/off
};

void remote_simple_light::print_name(){
	logger.pln(F("Remote simple light"));
};

// function called to adapt the state of the led
void remote_simple_light::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t px){
	// simplyfied to r=1/0 and push forward
	m_state.set(r);
	sprintf(m_msg_buffer,"dev%i/s/light",m_dev_id);
	if (r) {
		network.publish(m_msg_buffer,"ON");
		logger.println(TOPIC_INFO_SL, F("Remote simple light on"), COLOR_PURPLE);
	} else {
		network.publish(m_msg_buffer,"OFF");
		logger.println(TOPIC_INFO_SL, F("Remote simple light off"), COLOR_PURPLE);
	}
}
#endif
