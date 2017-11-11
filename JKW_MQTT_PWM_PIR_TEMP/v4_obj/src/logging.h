#ifndef logging_h
#define logging_h
#include <Arduino.h>

#define TOPIC_MQTT             1
#define TOPIC_MQTT_PUBLISH     2
#define TOPIC_MQTT_SUBSCIBED   3
#define TOPIC_MQTT_CONNECTED   4
#define	TOPIC_WIFI             5
#define	TOPIC_MQTT_IN          6
#define TOPIC_INFO_PWM         7
#define TOPIC_INFO_SL          8
#define TOPIC_BUTTON           9
#define TOPIC_GENERIC_INFO     10

#define COLOR_NONE		0
#define COLOR_RED 		1
#define COLOR_GREEN		2
#define COLOR_YELLOW	3
#define COLOR_PURPLE	4


class logging {
	public:
		logging();
		void print(uint8_t TOPIC,const __FlashStringHelper* text);
		void print(uint8_t TOPIC, char * text);
		void println(uint8_t TOPIC,const __FlashStringHelper* text);
		void println(uint8_t TOPIC, char * text);
		void print(uint8_t TOPIC,const __FlashStringHelper* text, uint8_t color);
		void print(uint8_t TOPIC, char * text, uint8_t color);
		void println(uint8_t TOPIC,const __FlashStringHelper* text, uint8_t color);
		void println(uint8_t TOPIC, char * text, uint8_t color);
		void topic(uint8_t TOPIC);
		void addColor(uint8_t color);
		void remColor(uint8_t color);
	private:
};

extern logging logger;

#endif
