#include "logging.h"

//342094
// 341550
logging::logging(){};

void logging::print(uint8_t TOPIC, const __FlashStringHelper* text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	Serial.print(text);
	remColor(color);
}

void logging::println(uint8_t TOPIC, const __FlashStringHelper* text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	Serial.println(text);
	remColor(color);
}

void logging::println(uint8_t TOPIC, char * text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	Serial.println(text);
	remColor(color);
};

void logging::print(uint8_t TOPIC, char * text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	Serial.print(text);
	remColor(color);
};

void logging::print(uint8_t TOPIC, const __FlashStringHelper* text){ print(TOPIC,text,0); }
void logging::println(uint8_t TOPIC, const __FlashStringHelper* text){ println(TOPIC,text,0);}
void logging::println(uint8_t TOPIC, char * text){ println(TOPIC,text,0); };
void logging::print(uint8_t TOPIC, char * text){	print(TOPIC,text,0); };

void logging::addColor(uint8_t color){
	/*
	RED='\033[0;31m'
	NC='\033[0m' # No Color
	Black        0;30     Dark Gray     1;30
	Red          0;31     Light Red     1;31
	Green        0;32     Light Green   1;32
	Brown/Orange 0;33     Yellow        1;33
	Blue         0;34     Light Blue    1;34
	Purple       0;35     Light Purple  1;35
	Cyan         0;36     Light Cyan    1;36
	Light Gray   0;37     White         1;37
	*/
	if(color==COLOR_RED){
		Serial.print("\033[0;31m");
	} else if(color==COLOR_GREEN){
		Serial.print("\033[0;32m");
	} else if(color==COLOR_YELLOW){
		Serial.print("\033[0;33m");
	} else if(color==COLOR_PURPLE){
		Serial.print("\033[0;35m");
	}

}
void logging::remColor(uint8_t color){
	if(color!=COLOR_NONE){
		Serial.print("\033[0m");
	}
}


void logging::topic(uint8_t TOPIC){
	if(TOPIC==TOPIC_MQTT){
		Serial.print(F("[MQTT]              "));
	} else if(TOPIC==TOPIC_MQTT_CONNECTED){
		Serial.print(F("[MQTT CONNECTED]    "));
	} else if(TOPIC==TOPIC_MQTT_SUBSCIBED){
		Serial.print(F("[MQTT subscribed]   "));
	} else if(TOPIC==TOPIC_MQTT_PUBLISH){
		Serial.print(F("[MQTT publish]      "));
	} else if(TOPIC==TOPIC_WIFI){
		Serial.print(F("[WiFi]              "));
	} else if(TOPIC==TOPIC_MQTT_IN){
		Serial.print(F("[MQTT in]           "));
	} else if(TOPIC==TOPIC_INFO_PWM){
		Serial.print(F("[INFO PWM]          "));
	} else if(TOPIC==TOPIC_INFO_SL){
		Serial.print(F("[INFO SL]           "));
	} else if(TOPIC==TOPIC_BUTTON){
		Serial.print(F("[BUTTON]            "));
	} else if(TOPIC==TOPIC_GENERIC_INFO){
		Serial.print(F("[GENERIC]           "));
	}
}
