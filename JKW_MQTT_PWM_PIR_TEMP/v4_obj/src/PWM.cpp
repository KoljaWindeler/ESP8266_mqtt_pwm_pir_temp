#include <PWM.h>

PWM::PWM(uint8_t* k, uint8_t pin0,uint8_t pin1, uint8_t pin2){
	m_pin0 = pin0;
	m_pin1 = pin1;
	m_pin2 = pin2;
	sprintf((char*)key,(char*)k);
};

PWM::~PWM(){
	uint8_t buffer[15];
	sprintf((char*)buffer,"%s deleted",get_key());
	logger.println(TOPIC_GENERIC_INFO, (char*)buffer, COLOR_YELLOW);
};
void PWM::interrupt(){};

uint8_t* PWM::get_key(){
	return key;
}

uint8_t* PWM::get_dep(){
	sprintf((char*)dep,"LIG");
	return dep;
}

bool PWM::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}

bool PWM::init(){
	pinMode(m_pin0, OUTPUT);
	pinMode(m_pin1, OUTPUT);
	pinMode(m_pin2, OUTPUT);
	analogWriteRange(255);
	sprintf(m_msg_buffer,"%s init, pin config %i,%i,%i",get_key(),m_pin0,m_pin1,m_pin2);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
}


bool PWM::loop(){
	return false;
}

uint8_t PWM::count_intervall_update(){
	return 0; // we have 0 value that we want to publish per minute
}

bool PWM::intervall_update(uint8_t slot){
	return false;
}

bool PWM::subscribe(){
	return true;
}

bool PWM::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false;
}

uint8_t PWM::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

void PWM::setState(uint8_t value){
	m_state.set(value);
}

bool PWM::publish(){
	// function called to publish the state of the PWM (on/off)
	return false;
}

void PWM::setColor(uint8_t r, uint8_t g, uint8_t b){
	// make sure that we never have a brightness of zero when we're switched on
	// rather set it to full brightness then to have it auto-off again
	//if (m_state.get_value() && r == 0) {
	//	r = sizeof(intens) - 1;
	//}
	analogWrite(m_pin0, r);
	analogWrite(m_pin1, g);
	analogWrite(m_pin2, b);

	logger.print(TOPIC_INFO_PWM, F("PWM: "));
	snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", r, g, b);
	logger.pln(m_msg_buffer);
} // setState
