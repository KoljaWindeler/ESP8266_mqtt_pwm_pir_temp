#include <AI.h>

my92x1 _my92x1 = my92x1(AI_DI_PIN, AI_DCKI_PIN, 4);


AI::AI(){};
AI::~AI(){
	logger.println(TOPIC_GENERIC_INFO, F("AI deleted"), COLOR_YELLOW);
};
void AI::interrupt(){};

bool AI::parse(uint8_t* config){
	return cap.parse(config,get_key(),(uint8_t*)"LIG");
}

my92x1* AI::getmy929x1(){
	return &_my92x1;
}

uint8_t* AI::get_key(){
	sprintf((char*)key,"AI");
	return key;
}

bool AI::init(){
	_my92x1.init(false); // false = AI
	logger.println(TOPIC_GENERIC_INFO, F("Button init"), COLOR_GREEN);
}


bool AI::loop(){
	return false; // i did nothing
}

uint8_t AI::count_intervall_update(){
	return 0; // we have 1 value that we want to publish per minute
}

void AI::setColor(uint8_t r, uint8_t g, uint8_t b){
	uint8_t w=0;
	if(r==b && b==g){
		w=r;
		r=0;
		g=0;
		b=0;
	}
	_my92x1.setColor((my92x1_color_t) { r, g, b, w, 0 });          // last two: warm white, cold white
}

uint8_t AI::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

bool AI::intervall_update(uint8_t slot){
	return false;
}

bool AI::subscribe(){
	return true;
}

bool AI::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool AI::publish(){
	return true;
}
