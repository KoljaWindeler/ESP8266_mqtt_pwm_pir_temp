#include <PWM.h>

PWM::PWM(){};
PWM::~PWM(){
	logger.println(TOPIC_GENERIC_INFO, F("PWM deleted"), COLOR_YELLOW);
};
void PWM::interrupt(){};

uint8_t* PWM::get_key(){
	sprintf((char*)key,"PWM");
	return key;
}

bool PWM::parse(uint8_t* config){
	return cap.parse(config,get_key(),(uint8_t*)"LIG");
}

bool PWM::init(){
	pinMode(PWM_LIGHT_PIN1, OUTPUT);
	pinMode(PWM_LIGHT_PIN2, OUTPUT);
	pinMode(PWM_LIGHT_PIN3, OUTPUT);
	analogWriteRange(255);
	logger.println(TOPIC_GENERIC_INFO, F("PWM init"), COLOR_GREEN);
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
	analogWrite(PWM_LIGHT_PIN1, r);
	analogWrite(PWM_LIGHT_PIN2, g);
	analogWrite(PWM_LIGHT_PIN3, b);

	logger.print(TOPIC_INFO_PWM, F("PWM: "));
	snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", r, g, b);
	logger.pln(m_msg_buffer);
} // setState
