#include "capability_parser.h"

// 342094
// 341550

capability_parser::capability_parser(){};


bool capability_parser::parse(unsigned char * input, uint8_t* key, uint8_t* dep){
	// in: P,S,...
	uint8_t p=0;
	uint8_t temp[10];
	uint8_t* cap_start;
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
	cap_start = input; // start of string
	while (*input) {
		if(*input!=','){
			temp[p]=*input;
			p++;
		} else {
			temp[p]=0x00;
			//Serial.printf("parsing. input: %s vs. key %s\r\n",temp,key);
			if(!strcmp((const char*)key,(const char*)temp)){ // if this is my key
				if(strcmp((const char*)dep,(const char*)"")){ // if i have a dependency
					if(ensure_dep(p_input,dep)){ // ensure depency is part of the config string or add it
						logger.print(TOPIC_GENERIC_INFO, F(""),COLOR_PURPLE);
						logger.addColor(COLOR_PURPLE);
						Serial.printf("%s added dependency %s\r\n",key,dep);
						logger.remColor(COLOR_PURPLE);
					}
				}
				// remove my token from config string, so only the non consumed token remain
				memset(cap_start, 'x', strlen((const char*)key));
				//Serial.println("parse return true");
				return true;
			}
			p=0;
			cap_start = input+1;
		}
		input++;
	}
	//Serial.println("parse return false");
	return false;
}

bool capability_parser::parse(unsigned char * input, uint8_t* key){
	return parse(input,key,(uint8_t*)"");
}

// e.g. to check if B4 is in the config, run: parse_wide(config,"B",&m_pin)
bool capability_parser::parse_wide(unsigned char* input, uint8_t* key_word, uint8_t* key_res){
	return parse_wide(input,"%s%i", key_word, 0, 16, key_res, (uint8_t*)"");
}

// e.g. to check if B4 is in the config and you have a dependency, run: parse_wide(config,"B",&m_pin,"LIG")
bool capability_parser::parse_wide(unsigned char* input, uint8_t* key_word, uint8_t* key_res, uint8_t* dep){
	return parse_wide(input,"%s%i", key_word, 0, 16, key_res, dep);
}

// input is the haystack
// key_schema will patch the key word and the key together, e.g. "%s%i"
// key_word is the first part e.g. "B"
// key_start defines the first pin nr
// key_end defines the last pin we're using
// key_res is a pointer, if we find something, we're going to write it to this pin
// dep is a dependency as always
bool capability_parser::parse_wide(unsigned char* input, const char* key_schema, uint8_t* key_word, uint8_t key_start, uint8_t key_end, uint8_t* key_res, uint8_t* dep){
	char temp_key[15]; // max key width is 15 byte (way to long)
	if(key_end>16){
		key_end = 16;
	}
	// loop over all possible pins (limit upper end to 16)
	for (uint8_t i = key_start; i <= key_end; i++) {
		if(i>=6 && i<=11){
			// gpio 6 to 11 should not be used, connection toward flash, controller will crash
			continue;
		}
		sprintf(temp_key, key_schema,key_word,i);
		if (cap.parse(input, (uint8_t *) temp_key , dep)) {
			*key_res = i;
			return true;
		}
	}
	return false;
}

bool capability_parser::ensure_dep(unsigned char* input, uint8_t* dep){
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
