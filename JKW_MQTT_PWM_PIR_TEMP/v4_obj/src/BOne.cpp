#include <BOne.h>

my92x1 _my92x1_b1 = my92x1(MY9291_DI_PIN, MY9291_DCKI_PIN,6);

BOne::BOne(){};
BOne::~BOne(){
	logger.println(TOPIC_GENERIC_INFO, F("B1 deleted"), COLOR_YELLOW);
};

void BOne::interrupt(){};

bool BOne::parse(uint8_t* config){
	return cap.parse(config,get_key(),(uint8_t*)"LIG");
}

bool BOne::init(){
	_my92x1_b1.init(true); // true = B1
	logger.println(TOPIC_GENERIC_INFO, F("B1 init"), COLOR_GREEN);
}

uint8_t* BOne::get_key(){
	sprintf((char*)key,"B1");
	return key;
}

my92x1* BOne::getmy929x1(){
	return &_my92x1_b1;
}

bool BOne::loop(){
	return false; // i did nothing
}

bool BOne::intervall_update(uint8_t slot){
	return false;
}

uint8_t BOne::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

void BOne::setColor(uint8_t r, uint8_t g, uint8_t b){
	uint8_t w=0;
	if(r==b && b==g){
		w=r;
		r=0;
		g=0;
		b=0;
	}
	_my92x1_b1.setColor((my92x1_color_t) { r, g, b, w, 0 });          // last two: warm white, cold white
}

bool BOne::subscribe(){
	return true;
}

uint8_t BOne::count_intervall_update(){
	return 0; // we have 1 value that we want to publish per minute
}

bool BOne::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool BOne::publish(){
	return true;
}
