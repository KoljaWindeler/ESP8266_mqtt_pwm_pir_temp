#ifndef audio_h
#define audio_h

#include "main.h"
#include "i2s.h"

// avconv -i Downloads/Madonna\ -\ Hung\ Up\ www.my-free-mp3.net\ .mp3 -f s32be -acodec pcm_u8 -ac 1 -ar 24000 tcp://192.168.2.59:5522

// avconv -i Downloads/Madonna\ -\ Hung\ Up\ www.my-free-mp3.net\ .mp3 -f s32be -acodec pcm_u8 -ac 1 -ar 22050 tcp://192.168.2.59:5522

// valid buffer Sizes are e.g. 0x1000, 0x2000, 0x4000
#define BUFFER_SIZE    	0x4000
#define AMP_ENABLE_PIN 	15
#define AUDIO_PORT 			5522
static constexpr char MQTT_AUDIO_TOPIC[]           = "audio";         // publish


enum RAMP_DIRECTIONS { DOWN, UP };
enum PWM_MODES { PWM_RESET, PWM_NORMAL, PWM_DIRECT };


class audio : public peripheral {
	public:
		audio();
		~audio();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
	private:

		WiFiServer *server;
		uint8_t *buffer8b;
		uint16_t bufferPtrIn;
		uint16_t bufferPtrOut;
		uint32_t ultimeout;

		uint8_t gainF2P6; // Fixed point 2.6
		typedef int32_t fixed24p8_t;
		fixed24p8_t lastSamp; // Last sample value
    fixed24p8_t cumErr;   // Running cumulative error since time began
		enum {fixedPosValue=0x007fff00}; /* 24.8 of max-signed-int */
		bool ConsumeSample(int16_t sample);
		int16_t Amplify(int16_t s);

		uint8_t key[3];
		inline void doPWM(uint8_t ctrl, uint8_t value8b);
		//void rampPWM(uint8_t direction);
		void rampPWM(uint8_t direction,uint16_t target);
		inline void startStreaming(WiFiClient *client);
};


#endif
