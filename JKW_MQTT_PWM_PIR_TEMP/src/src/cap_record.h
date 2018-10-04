#ifndef record_h
#define record_h

// based on large parts on https://github.com/sven337/jeenode/blob/master/babymonitor/recv/recv.ino

#include "main.h"
#include <SPI.h>
#include <WiFiUdp.h>

static constexpr char MQTT_RECORD_TOPIC[]   = "topic1";

#define TARGET_PORT 45990
#define CS_PIN = 15; // d8
#define SILENCE_EMA_WEIGHT 1024
#define REC_BUFFER_SIZE 700
#define ENVELOPE_EMA_WEIGHT 2
#define ICACHE_RAM_ATTR     __attribute__((section(".iram.text")))

class record : public peripheral {
	public:
		record();
		~record();
		bool init();
		bool loop();
		bool intervall_update(uint8_t slot);
		bool subscribe();
		bool parse(uint8_t* config);
		uint8_t count_intervall_update();
		bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();
		uint8_t* get_key();
		void sample_isr();

	private:
		uint8_t key[3];
		WiFiClient tcp_client;
		uint16_t* adc_buf; // ADC data buffer, double buffered
		bool current_adc_buf; // which data buffer is being used for the ADC (the other is being sent)
		unsigned int adc_buf_pos; // position in the ADC data buffer
		bool send_samples_now; // flag to signal that a buffer is ready to be sent
		int32_t silence_value; // computed as an exponential moving average of the signal
		uint16_t envelope_threshold; // envelope threshold to trigger data sending
		uint32_t send_sound_util; // date until sound transmission ends after an envelope threshold has triggered sound transmission
		bool enable_highpass_filter;

		static inline ICACHE_RAM_ATTR uint16_t transfer16(void);
		void spiBegin(void);
		static inline void setDataBits(uint16_t bits);
		uint8_t* delta7_sample(uint16_t last, uint16_t *readptr, uint8_t *writeptr);
};


#endif
