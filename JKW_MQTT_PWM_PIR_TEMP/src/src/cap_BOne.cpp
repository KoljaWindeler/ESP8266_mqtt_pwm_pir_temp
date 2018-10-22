#include <cap_BOne.h>
#ifdef WITH_BONE

BOne::BOne(){};
BOne::~BOne(){
	logger.println(TOPIC_GENERIC_INFO, F("B1 deleted"), COLOR_YELLOW);
};

bool BOne::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}

bool BOne::init(){
	_my92x1_b1.init(true,MY9291_DI_PIN, MY9291_DCKI_PIN,6); // true = B1
	logger.println(TOPIC_GENERIC_INFO, F("B1 init"), COLOR_GREEN);
}

uint8_t* BOne::get_dep(){
	return (uint8_t*)"LIG";
}

uint8_t* BOne::get_key(){
	sprintf((char*)key,"B1");
	return key;
}

my92x1* BOne::getmy929x1(){
	return &_my92x1_b1;
}

// bool BOne::loop(){
// 	return false; // i did nothing
// }
//
// bool BOne::intervall_update(uint8_t slot){
// 	return false;
// }

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

// bool BOne::subscribe(){
// 	return true;
// }
//
//
// bool BOne::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
// bool BOne::publish(){
// 	return false; // i did notihgin
// }
#endif
