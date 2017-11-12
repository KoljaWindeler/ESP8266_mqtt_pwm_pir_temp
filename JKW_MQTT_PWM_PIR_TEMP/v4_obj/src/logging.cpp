#include "logging.h"

//342094
// 341550
logging::logging(){
	init();
};

void logging::init(){
	head=0;
	tail=0;
	msg_c=0;
};

void logging::print(uint8_t TOPIC, const __FlashStringHelper* text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	p(text);
	remColor(color);
}

void logging::println(uint8_t TOPIC, const __FlashStringHelper* text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	pln(text);
	remColor(color);
}

void logging::println(uint8_t TOPIC, char * text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	pln(text);
	remColor(color);
};

void logging::print(uint8_t TOPIC, char * text, uint8_t color){
	addColor(color);
	topic(TOPIC);
	p(text);
	remColor(color);
};


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
		p((char*)"\033[0;31m");
	} else if(color==COLOR_GREEN){
		p((char*)"\033[0;32m");
	} else if(color==COLOR_YELLOW){
		p((char*)"\033[0;33m");
	} else if(color==COLOR_PURPLE){
		p((char*)"\033[0;35m");
	}

}
void logging::remColor(uint8_t color){
	if(color!=COLOR_NONE){
		p((char*)"\033[0m");
	}
}


void logging::topic(uint8_t TOPIC){
	if(TOPIC==TOPIC_MQTT){
		p(F("[MQTT]              "));
	} else if(TOPIC==TOPIC_MQTT_CONNECTED){
		p(F("[MQTT CONNECTED]    "));
	} else if(TOPIC==TOPIC_MQTT_SUBSCIBED){
		p(F("[MQTT subscribed]   "));
	} else if(TOPIC==TOPIC_MQTT_PUBLISH){
		p(F("[MQTT publish]      "));
	} else if(TOPIC==TOPIC_WIFI){
		p(F("[WiFi]              "));
	} else if(TOPIC==TOPIC_MQTT_IN){
		p(F("[MQTT in]           "));
	} else if(TOPIC==TOPIC_INFO_PWM){
		p(F("[INFO PWM]          "));
	} else if(TOPIC==TOPIC_INFO_SL){
		p(F("[INFO SL]           "));
	} else if(TOPIC==TOPIC_BUTTON){
		p(F("[BUTTON]            "));
	} else if(TOPIC==TOPIC_GENERIC_INFO){
		p(F("[GENERIC]           "));
	}
}

void logging::print(uint8_t TOPIC, const __FlashStringHelper* text){ print(TOPIC,text,0); }
void logging::println(uint8_t TOPIC, const __FlashStringHelper* text){ println(TOPIC,text,0);}
void logging::println(uint8_t TOPIC, char * text){ println(TOPIC,text,0); };
void logging::print(uint8_t TOPIC, char * text){	print(TOPIC,text,0); };

/////////////////////////////////////////////////////////////////
void logging::p(const __FlashStringHelper* t){
	addChar(String(t));
}

void logging::p(char * t){
	addChar(String(t));
};

void logging::p(uint8_t t){
	addChar(String(t));
};

void logging::pln(uint8_t t){
	addChar(String(t));
	addChar('\n');
};

void logging::pln(char * t){
	addChar(String(t));
	addChar('\n');
};

void logging::pln(const __FlashStringHelper* t){
	addChar(String(t));
	addChar('\n');

}
/////////////////////////////////////////////////////////////////
void logging::addChar(String S){
	for(uint16_t i=0; i<S.length(); i++){
		addChar(S.charAt(i));
	}
}

void logging::addChar(uint8_t c){
	if(c=='\r'){
		return;
	} else if(c=='\n'){
		Serial.println("");
		c=0x00;
	} else if(c!=0x00){
		Serial.print((char)c);
	}

	buffer[(tail)%(LOGGING_BUFFER_SIZE-1)]=c;
	tail=(tail+1)%(LOGGING_BUFFER_SIZE-1); // 0..498
	buffer[(tail)%(LOGGING_BUFFER_SIZE-1)]=0x00;

	//if(tail==0){
	//	Serial.print("\r\nwrap\r\n");
	//}

	if(tail==head){
		head=(tail+1)%(LOGGING_BUFFER_SIZE-1); // 0..498
	}
	// search last 0x00
	msg_c++;
	if(c==0x00){
		msg_c=0;
	} else if(msg_c>100){
		msg_c=0;
		addChar(0x00);
		addChar('$');
		addChar('$');
	}
}

uint16_t logging::available(){
	if(head==tail){
		return 0;
	} else if(head<tail){
		return tail-head;
	} else {
		return LOGGING_BUFFER_SIZE-head+tail;
	}
}

uint8_t* logging::loop(){
	uint32_t ret;
	if(head!=tail){
		//LOGGING_BUFFER_SIZE
		//uint8_t b[100];
		ret = (uint32_t)buffer + head;
		head = (head+strlen((const char*)ret)+1)%LOGGING_BUFFER_SIZE;
		if(head==tail+1){
			tail=head;
		}
		//Serial.printf("h %i t %i\r\n",head,tail);
		return (uint8_t*)ret;
	}
	return 0;
}
