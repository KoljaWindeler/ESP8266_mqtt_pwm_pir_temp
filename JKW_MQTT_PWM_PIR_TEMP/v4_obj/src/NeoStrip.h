#ifndef NeoStrip_h
#define NeoStrip_h

#include <Arduino.h>
#include "mqtt_parameter.h"
#include "main.h"
#include <NeoPixelBus.h>

class NeoStrip : public peripheral {
	public:
		NeoStrip();
		~NeoStrip();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		void interrupt();
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();

		void setColor(uint8_t r, uint8_t g, uint8_t b);
		uint8_t getState(led* color);
		void setState(uint8_t value);
		void show();
		void setPixelColor(uint8_t r, uint8_t g, uint8_t b, uint8_t px);
	private:
		uint8_t key[4];
		mqtt_parameter_8 m_state;
		led m_light_current;
		//NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(NEOPIXEL_LED_COUNT, PWM_LIGHT_PIN2); // this version only works on gpio3 / D9 (RX)
};


#endif
