#ifndef logging_h
	#define logging_h
#include <Arduino.h>

	#define TOPIC_MQTT            1
	#define TOPIC_MQTT_PUBLISH    2
	#define TOPIC_MQTT_SUBSCIBED  3
	#define TOPIC_MQTT_CONNECTED  4
	#define TOPIC_WIFI            5
	#define TOPIC_MQTT_IN         6
	#define TOPIC_INFO_PWM        7
	#define TOPIC_INFO_SL         8
	#define TOPIC_BUTTON          9
	#define TOPIC_GENERIC_INFO    10
	#define TOPIC_CON_REL         11
	#define TOPIC_OTA             12

	#define COLOR_NONE            0
	#define COLOR_RED             1
	#define COLOR_GREEN           2
	#define COLOR_YELLOW          3
	#define COLOR_PURPLE          4

	#define LOGGING_BUFFER_SIZE   1500
	#define EXCEPT_BUFFER_WASTING 0.03
	#define MAX_MQTT_MSG          100


	class logging {
public:
		logging();
		void init();
		void print(uint8_t TOPIC, const __FlashStringHelper * text);
		void print(uint8_t TOPIC, char * text);
		void println(uint8_t TOPIC, const __FlashStringHelper * text);
		void println(uint8_t TOPIC, char * text);
		void print(uint8_t TOPIC, const __FlashStringHelper * text, uint8_t color);
		void print(uint8_t TOPIC, char * text, uint8_t color);
		void println(uint8_t TOPIC, const __FlashStringHelper * text, uint8_t color);
		void println(uint8_t TOPIC, char * text, uint8_t color);
		void resetSerialLine();
		void topic(uint8_t TOPIC);
		void addColor(uint8_t color);
		void remColor(uint8_t color);
		// print and println
		void p(char * t);
		void pln(char * t);
		void p(uint8_t i);
		void pln(uint8_t i);
		void p(const __FlashStringHelper * t);
		void pln(const __FlashStringHelper * t);
		uint8_t * loop();
		void addChar(uint8_t c);
		void addChar(String S);
		void enable_mqtt_trace(bool in);
		void enable_serial_trace(bool in);
private:
		uint8_t buffer[LOGGING_BUFFER_SIZE];
		uint16_t head;
		uint16_t tail;
		uint8_t msg_c;
		bool serial_trace_active;
		bool mqtt_trace_active;
	};

	extern logging logger;

#endif // ifndef logging_h
