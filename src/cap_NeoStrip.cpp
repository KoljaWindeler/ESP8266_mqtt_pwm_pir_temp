#ifdef WITH_NEOSTRIP
#include <cap_NeoStrip.h>

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NEOPIXEL_LED_COUNT, 5); // this version only works on gpio3 / D9 (RX)

NeoStrip::NeoStrip(){};
NeoStrip::~NeoStrip(){
	logger.println(TOPIC_GENERIC_INFO, F("NEO deleted"), COLOR_YELLOW);
};

uint8_t* NeoStrip::get_key(){
	return (uint8_t*)"NEO";
}

uint8_t* NeoStrip::get_dep(){
	return (uint8_t*)"LIG";
}

bool NeoStrip::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}

bool NeoStrip::init(){
	strip.Begin();
	logger.println(TOPIC_GENERIC_INFO, F("NEO init"), COLOR_GREEN);
	return true;
}

// bool NeoStrip::loop(){																					return false; }// i did nothing
// bool NeoStrip::intervall_update(uint8_t slot){									return false; }
// bool NeoStrip::subscribe(){																			return true;  }
// bool NeoStrip::receive(uint8_t* p_topic, uint8_t* p_payload){		return false; }// not for me
// bool NeoStrip::publish(){																				return false; } // noting to say


uint8_t NeoStrip::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

void NeoStrip::setState(uint8_t value){
	m_state.set(value);
}

uint8_t NeoStrip::get_modes(){
	return (1<<SUPPORTS_PWM) | (1<<SUPPORTS_RGB) | (1<<SUPPORTS_PER_PIXEL); 
};

void NeoStrip::print_name(){
	logger.pln(F("Shelly dimmer"));
};


void NeoStrip::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t px){
	if(px==255){
		for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
			strip.SetPixelColor(i, RgbColor(r,g,b));
		}
		strip.Show();
	} else {
		strip.SetPixelColor(px, RgbColor(r,g,b));
	}
}

void NeoStrip::show(){
	strip.Show();
}

#endif
