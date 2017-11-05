#include "capability.h"

// 342094
// 341550

capability::capability(){};


bool capability::parse(unsigned char * input, uint8_t* key){
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
				return true;
			}
			p=0;
		}
		input++;
	}
	return false;
}
