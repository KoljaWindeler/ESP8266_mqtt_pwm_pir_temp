#include "capability.h"

// 342094
// 341550

capability::capability(){};


bool capability::parse(unsigned char * input, uint8_t* key, uint8_t* dep){
	// in: P,S,...
	uint8_t p=0;
	uint8_t len = 0;
	uint8_t temp[10];
	unsigned char * p_input=input;

	// add trainig comma
	while(input[p]){
		p++;
	}
	if(input[p-1]!=','){
		sprintf((char*)input,"%s,",input);
	}
	//Serial.printf("cap parsing. input: %s vs. key %s\r\n",input,key);
	p=0;
	while (*input) {
		if(*input!=','){
			temp[p]=*input;
			p++;
		} else {
			temp[p]=0x00;
			//Serial.printf("parsing. input: %s vs. key %s\r\n",temp,key);
			if(!strcmp((const char*)key,(const char*)temp)){
				if(strcmp((const char*)dep,(const char*)"")){
					if(ensure_dep(p_input,dep)){
						logger.print(TOPIC_GENERIC_INFO, F(""),COLOR_PURPLE);
						logger.addColor(COLOR_PURPLE);
						Serial.printf("%s added dependency %s\r\n",key,dep);
						logger.remColor(COLOR_PURPLE);
					}
				}
				//Serial.println("parse return true");
				return true;
			}
			p=0;
		}
		input++;
	}
	//Serial.println("parse return false");
	return false;
}

bool capability::parse(unsigned char * input, uint8_t* key){
	return parse(input,key,(uint8_t*)"");
}

bool capability::ensure_dep(unsigned char* input, uint8_t* dep){
	//Serial.printf("ensure parsing. input: %s vs. dep %s\r\n",input,dep);
	if(parse(input,dep)){
		//Serial.println("ensure return false");
		return false;
	} else {
		// make sure there is a trailing comma
		uint8_t i = 0;
		while(i<100 && input[i]!=0x00){// arg hard coding
			i++;
		}
		if(input[i-1]!=','){
			input[i]=',';
			input[i+1]=0x00;
		}

		sprintf((char*)input,"%s%s",input,dep);
		//Serial.println("ensure return true");
		return true;
	}
}
