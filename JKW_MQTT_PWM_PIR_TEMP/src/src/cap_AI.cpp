#include <cap_AI.h>

#ifdef WITH_AI


AI::AI(){};
AI::~AI(){
	logger.println(TOPIC_GENERIC_INFO, F("AI deleted"), COLOR_YELLOW);
};

bool AI::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}


uint8_t* AI::get_dep(){
	return (uint8_t*)"LIG";
}

my92x1* AI::getmy929x1(){
	return &_my92x1;
}

uint8_t* AI::get_key(){
	return (uint8_t*)"AI";
}

bool AI::init(){
	_my92x1 = my92x1();
	_my92x1.init(false,AI_DI_PIN, AI_DCKI_PIN, 4); // false = AI
	logger.println(TOPIC_GENERIC_INFO, F("AI init"), COLOR_GREEN);
}


// bool AI::loop(){
// 	return false; // i did nothing
// }


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

// bool AI::subscribe(){
// 	return true;
// }
//
// bool AI::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
// bool AI::publish(){
// 	return false; // i did notihgin
// }

#endif
