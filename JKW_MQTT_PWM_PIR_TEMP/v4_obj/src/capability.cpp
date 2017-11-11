#include "capability.h"

// 342094
// 341550

capability::capability(){};


bool capability::parse(unsigned char * input, uint8_t* key, uint8_t* dep){
	// in: P,S,...
	uint8_t p=0;
	uint8_t len = 0;
	uint8_t temp[10];

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
					if(ensure_dep(input,dep)){
						logger.print(TOPIC_GENERIC_INFO, F(""),COLOR_PURPLE);
						logger.addColor(COLOR_PURPLE);
						Serial.printf("%s added dependency %s\r\n",key,dep);
						logger.remColor(COLOR_PURPLE);
					}
				}
				return true;
			}
			p=0;
		}
		input++;
	}
	return false;
}

bool capability::parse(unsigned char * input, uint8_t* key){
	parse(input,key,(uint8_t*)"");
}

bool capability::ensure_dep(unsigned char* input, uint8_t* dep){
	if(parse(input,dep)){
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
		return true;
	}
}
