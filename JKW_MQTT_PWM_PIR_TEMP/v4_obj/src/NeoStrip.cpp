#include <NeoStrip.h>

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NEOPIXEL_LED_COUNT, PWM_LIGHT_PIN2); // this version only works on gpio3 / D9 (RX)

NeoStrip::NeoStrip(){};
NeoStrip::~NeoStrip(){
	logger.println(TOPIC_GENERIC_INFO, F("NEO deleted"), COLOR_YELLOW);
};

uint8_t* NeoStrip::get_key(){
	sprintf((char*)key,"NEO");
	return key;
}

bool NeoStrip::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool NeoStrip::init(){
	strip.Begin();
	logger.println(TOPIC_GENERIC_INFO, F("NEO init"), COLOR_GREEN);
	return true;
}

bool NeoStrip::loop(){																					return false; }// i did nothing
bool NeoStrip::intervall_update(uint8_t slot){									return false; }
bool NeoStrip::subscribe(){																			return true;  }
bool NeoStrip::receive(uint8_t* p_topic, uint8_t* p_payload){		return false; }// not for me
bool NeoStrip::publish(){																				return true;  }
void NeoStrip::interrupt(){};
uint8_t NeoStrip::count_intervall_update(){											return 0;		 	};

uint8_t NeoStrip::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

void NeoStrip::setState(uint8_t value){
	m_state.set(value);
}


void NeoStrip::setColor(uint8_t r, uint8_t g, uint8_t b){ // 0..9
	// limit max
	/*
	if (r >= sizeof(intens)) {
		r = sizeof(intens) - 1;
	}
	if (g >= sizeof(intens)) {
		g = sizeof(intens) - 1;
	}
	if (b >= sizeof(intens)) {
		b = sizeof(intens) - 1;
	}

	m_light_current.r = r;
	m_light_current.g = g;
	m_light_current.b = b;
	*/
	for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
		strip.SetPixelColor(i, RgbColor(r,g,b));
	}
	strip.Show();
}

void NeoStrip::setPixelColor(uint8_t r, uint8_t g, uint8_t b, uint8_t px){
	strip.SetPixelColor(px, RgbColor(r,g,b));
}

void NeoStrip::show(){
	strip.Show();
}
