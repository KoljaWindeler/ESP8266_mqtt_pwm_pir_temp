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
	mqtt_trace_active=false;
	serial_trace_active=true;
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


void logging::enable_mqtt_trace(bool in){
	mqtt_trace_active=in;
}

void logging::enable_serial_trace(bool in){
	serial_trace_active=in;
}

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
		if(serial_trace_active){
			Serial.println("");
		}
		c=0x00;
	} else if(c!=0x00){
		if(serial_trace_active){
			Serial.print((char)c);
		}
	}

	if(!mqtt_trace_active){
		return;
	}

	buffer[(tail)%(LOGGING_BUFFER_SIZE-1)]=c;
	tail=(tail+1)%(LOGGING_BUFFER_SIZE-1); // 0..498
	buffer[(tail)%(LOGGING_BUFFER_SIZE-1)]=0x00;

	// tail = pointer auf die 0x00 hinter dem string
	// head = pointer auf das erste byte das noch zu senden ist

	if(tail==0){
		//Serial.print("\r\nwrap\r\n");
		uint16_t lnb=LOGGING_BUFFER_SIZE;
		for(uint8_t i=1; i<EXCEPT_BUFFER_WASTING*LOGGING_BUFFER_SIZE; i++){
			if(buffer[LOGGING_BUFFER_SIZE-1-i]==0x00){
				lnb=LOGGING_BUFFER_SIZE-i; //-1+1
				break;
			}
		}
		//Serial.printf("lnb eos2=%i\r\n",lnb);
		//Serial.println((char*)(buffer+lnb));

		if(lnb<LOGGING_BUFFER_SIZE){ // aka there was another 0x00 before the end
			//Serial.printf("cpy/set\r\n");
			memcpy(buffer, (uint8_t*)(buffer+lnb), LOGGING_BUFFER_SIZE-lnb-1);
			//Serial.println((char*)buffer);
			memset((uint8_t*)(buffer+lnb), 0x00, LOGGING_BUFFER_SIZE-lnb);
			tail=LOGGING_BUFFER_SIZE-lnb-1;
			// tail wraped now, is now >0. Head should be somewhere in the middle
			// except we just wrapped .. at that poin the head < tail
			if(head<=tail){
				head=tail+1;// -1+1
			}
		}
	}

	if(tail==head){
		head=(tail+1)%(LOGGING_BUFFER_SIZE-1); // 0..498
		// visulize buffer overrun with 2x#
		buffer[head]='#';
		buffer[(head+1)%(LOGGING_BUFFER_SIZE-1)]='#';
	}
	// search last 0x00
	msg_c++;
	if(c==0x00){
		msg_c=0;
	} else if(msg_c>MAX_MQTT_MSG){
		msg_c=0;
		addChar(0x00);
		addChar('$');
		addChar('$');
	}
}


uint8_t* logging::loop(){
	if(!mqtt_trace_active){
		return 0;
	}
	uint32_t ret;
	if(head!=tail){
		ret = (uint32_t)buffer + head;
		//Serial.printf("h %i t %i: %s[$] %i\r\n",head,tail,(char*)(ret),*((uint8_t*)ret));

		// assume buffer ist KOLJA[0x00], head is pointing to "K" strlen will return 5
		// head + 5 points to the 0x00 behind kolja, thus add one to get behind it
		head = (head+strlen((const char*)ret)+1)%LOGGING_BUFFER_SIZE;
		// but if the tail was pointing to the 0x00 marker we've just passed we'll have head = tail+1
		// and loop to buffer again, that must be stopped
		if(head==tail+1){
			tail=head;
		}
		//Serial.printf("A h:%i\r\n",head);
		// is the string on this buffer is directly followed by another 0x00
		// simply skip this buffer (return 0 will avoid sending)
		if(strlen((char*)(ret))==0){
			return 0;
		}
		return (uint8_t*)ret;
	}
	return 0;
}
