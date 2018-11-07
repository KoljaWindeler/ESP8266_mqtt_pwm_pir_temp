#include "mqtt_parameter.h"

mqtt_storage::mqtt_storage(){};

void mqtt_storage::init(){
	// uint32_t      t=millis();

	m_mqtt_sizes[0] = sizeof(m_mqtt->login);
	m_mqtt_sizes[1] = sizeof(m_mqtt->pw);
	m_mqtt_sizes[2] = sizeof(m_mqtt->dev_short);
	m_mqtt_sizes[3] = sizeof(m_mqtt->server_ip);
	m_mqtt_sizes[4] = sizeof(m_mqtt->server_port);
	m_mqtt_sizes[5] = sizeof(m_mqtt->nw_ssid);
	m_mqtt_sizes[6] = sizeof(m_mqtt->nw_pw);
	m_mqtt_sizes[7] = sizeof(m_mqtt->cap);

	f_p = 0;                  // pointer in dem struct
	p   = (uint8_t *) m_mqtt; // copy location
	skip_last = 1; // skip capability
}

void mqtt_storage::loop(){
	// ////////////// kolja serial config code ////////////
	// if(millis()-t>1000){
	//  t=millis();
	//  Serial.print("+");
	// }

	if (Serial.available()) {
		char_buffer = Serial.read();
		// Serial.print(char_buffer);
		// start char for config
		if (char_buffer == '?') {
			// Serial.print("@");
			p = (uint8_t *) m_mqtt;
			f_p         = 0;
			char_buffer = 255; // enter the "if" below
			f_start     = 0;
		}

		// jump to next field
		if (char_buffer == 13 || char_buffer == 255) {
			// Serial.print("#");
			f_start = 0;
			for (int i = 0; i <= sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]) - skip_last; i++) { // 1.2.3.4.5.6.7
				if (i > 0) {
					f_start += m_mqtt_sizes[i - 1];
				}
				// Serial.printf("+%i %i\r\n",f_p,f_start);
				// no text entered but [enter], keep content
				if(f_p == f_start && char_buffer == 13){
					strcpy((char*)p,getMQTTelement(i,m_mqtt));
					f_p+=strlen(getMQTTelement(i,m_mqtt));
					p+=strlen(getMQTTelement(i,m_mqtt));
					Serial.print(getMQTTelement(i,m_mqtt));
				}
				// seach for the field that starts closes to our current pos
				if (f_p <= f_start) {
					for (int ii = 0; ii < f_start - f_p; ii++) { // add as many 0x00 to the config as required
						*p = 0x00;
						p++;
					}
					f_p = f_start; // set our new pos to the start of that field
					// print some shiny output
					if (i == 0) {
						Serial.print(F("\r\n\r\nStart readig config"));
					};
					if (i >= 0 && i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]) - skip_last) {
						Serial.println("");
						explainMqttStruct(i, false);
						Serial.printf("[%s]\r\n",getMQTTelement(i,m_mqtt));
					} else if (i == sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]) - skip_last) { // last segement .. save and reboot
						// fill the buffer
						Serial.print(F("\r\n==========\r\nConfig stored\r\n"));
						explainFullMqttStruct(m_mqtt);
						Serial.print(F("==========\r\n"));
						// write to address 0 ++
						f_start = 0;
						for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]); i++) { // 1.2.3.4.5.6.7
							f_start += m_mqtt_sizes[i];
						}
						storeMqttStruct((char *) m_mqtt, f_start);
						delay(1000);

						// what about the wifi?
						Serial.print(F("Disconnect.\r\n"));
						WiFi.disconnect(false);
						Serial.print(F("Connect.\r\n"));
						WiFi.begin(m_mqtt->nw_ssid, m_mqtt->nw_pw);
						Serial.print(F("checking.\r\n"));
						if (WiFi.waitForConnectResult() != WL_CONNECTED) {
							Serial.print(F("Failed to connect."));
						} else {
							Serial.print(F("Connect ok. Restart now"));
							delay(500);
							ESP.restart();
						}
					}
					break; // leave loop
				}     // if(fp<f_start)
			} // loop over struct size
				// set pointer to start of the field
			p  = (uint8_t *) m_mqtt;
			p += f_p;
		} else if (char_buffer == 127) { // backspace
			// search lowerlimit of this field
			f_start = 0;
			for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]) - skip_last; i++) { // 0.1.2.3.4.5.6.7
				// Serial.print("+");
				if (f_start + m_mqtt_sizes[i] > f_p) { // seach for the field that starts closes to our current pos
					break;
				}
				f_start += m_mqtt_sizes[i];
			}
			// Serial.printf("%i--%i\r\n",f_p,f_start);
			if (f_p > f_start) {
				p--; // limits?
				f_p--;
				Serial.print((char) 0x08); // geht das? ulkig aber ja
			}
		} else if (char_buffer != 10) { // plain char storing "\r"
			// Serial.print("&");
			// calc next segment
			f_start = 0;
			for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]) - skip_last; i++) { // 0.1.2.3.4.5.6.7
				// Serial.print("+");
				f_start += m_mqtt_sizes[i];
				if (f_p < f_start) {  // seach for the field that starts closes to our current pos
					break;
				}
			}
			// if(f_p<sizeof(*m_mqtt)-1){ // go on as long as we're in the structure
			if (f_p < f_start - 1) { // go on as long as we're in the structure
				// e.g.: first field is 16 byte long (f_start=16), we can use [0]..[14], [15] must be 0x00, so 13<16-1, 14<16-1; !! 15<16-1
				if (char_buffer != '\r' && char_buffer != '\n') {
					Serial.print((char) char_buffer);
				}
				*p = char_buffer; // store incoming char in struct
				p++;
				f_p++;
				// } else {
				//  Serial.println("L");
			}
		}
		//connect = false; ?
	}
	// ////////////// kolja serial config code ////////////
}

void mqtt_storage::setMqtt(mqtt_data * mqtt){
	EEPROM.begin(512); // can be up to 4096
	m_mqtt = mqtt;
}

// default entry points
boolean mqtt_storage::storeMqttStruct(char * temp, uint8_t size){
	//return true;
	//return storeMqttStruct_universal(temp, sizeof(mqtt_data_v3),CHK_FORMAT_V3);
	return storeMqttStruct_universal(temp, LEN_FORMAT_V4, CHK_FORMAT_V4);
}
boolean mqtt_storage::loadMqttStruct(char * temp, uint8_t size){
	//return loadMqttStruct_v2(temp, sizeof(mqtt_data_v2));
	//return loadMqttStruct_v3(temp, sizeof(mqtt_data_v3));
	return loadMqttStruct_universal(temp, LEN_FORMAT_V4, CHK_FORMAT_V4);
}

boolean mqtt_storage::storeMqttStruct_universal(char * temp, uint8_t size, uint8_t chk){
	EEPROM.begin(512); // can be up to 4096
	uint8_t checksum = chk;
	//return true; // debug first
	for (int i = 0; i < size; i++) {
		EEPROM.write(i, *temp);
		// Serial.print(int(*temp));
		checksum ^= *temp;
		temp++;
	}
	EEPROM.write(size, checksum);
	// double write check, starting with v4
	if(chk >= CHK_FORMAT_V4){
		EEPROM.write(size+1, checksum);
	}
	EEPROM.commit();
	Serial.printf("%i byte written\r\n",size);
	return true;
}

boolean mqtt_storage::loadMqttStruct_universal(char * p_mqtt, uint8_t size, uint8_t chk){
	EEPROM.begin(512); // can be up to 4096
	uint8_t checksum = chk;
	char* temp=p_mqtt;
	for (int i = 0; i < size; i++) {
		*p_mqtt = EEPROM.read(i);
		//Serial.printf(" (%i)",i);
		//Serial.print(char(*p_mqtt));
		checksum ^= *p_mqtt;
		p_mqtt++;
	}
	uint8_t c1 = EEPROM.read(size);
	uint8_t c2 = EEPROM.read(size+1);

	checksum ^= c1;
	if (checksum == 0x00 && c1 == c2) {
		// Serial.println("EEok");
		return true;
	} else {
		p_mqtt=temp;
		sprintf(((mqtt_data*)p_mqtt)->dev_short, "new");
		sprintf(((mqtt_data*)p_mqtt)->server_ip, "1.2.3.4");
		sprintf(((mqtt_data*)p_mqtt)->server_port, "1883");
		sprintf(((mqtt_data*)p_mqtt)->login, "new");
		sprintf(((mqtt_data*)p_mqtt)->pw, "new");
		sprintf(((mqtt_data*)p_mqtt)->cap, "");
		sprintf(((mqtt_data*)p_mqtt)->nw_ssid, "new");
		sprintf(((mqtt_data*)p_mqtt)->nw_pw, "new");
		storeMqttStruct_universal(p_mqtt, size, chk);
		return false;
	}
}

char* mqtt_storage::getMQTTelement(uint8_t i, mqtt_data * mqtt){
	if(i==0){
		return mqtt->login;
	} else if(i==1){
		return mqtt->pw;
	} else if(i==2){
		return mqtt->dev_short;
	} else if(i==3){
		return mqtt->server_ip;
	} else if(i==4){
		return mqtt->server_port;
	} else if(i==5){
		return mqtt->nw_ssid;
	} else if(i==6){
		return mqtt->nw_pw;
	}
	return mqtt->cap;
}

void mqtt_storage::explainFullMqttStruct(mqtt_data * mqtt){
	for(uint8_t i=0; i<sizeof(mqtt->cap);i++){
		if((mqtt->cap[i]<'A' || mqtt->cap[i]>'z') && mqtt->cap[i]!=',' && (mqtt->cap[i]<'0' || mqtt->cap[i]>'9')){
			//Serial.printf("Replacing %c\r\n",mqtt->cap[i]);
			mqtt->cap[i]=0x00;
		}
	}
	for(uint8_t i=0; i<8; i++){
		explainMqttStruct(i, false);
		Serial.println(getMQTTelement(i,mqtt));
	}
}

void mqtt_storage::explainMqttStruct(uint8_t i, boolean rn){
	if (rn) {
		Serial.print(F("\r\n"));
	}
	if (i == 0) {
		Serial.print(F("MQTT login: "));
	} else if (i == 1) {
		Serial.print(F("MQTT pw: "));
	} else if (i == 2) {
		Serial.print(F("MQTT dev_short: "));
	} else if (i == 3) {
		Serial.print(F("MQTT server IP: "));
	} else if (i == 4) {
		Serial.print(F("MQTT server port: "));
	} else if (i == 5) {
		Serial.print(F("Network SSID: "));
	} else if (i == 6) {
		Serial.print(F("Network PW: "));
	} else if (i == 7) {
		Serial.print(F("MQTT Capability: "));
	} else {
		Serial.print(F("ERROR 404 "));
	}
	if (rn) {
		Serial.print(F("\r\n"));
	}
};
////////////////////////////////////////////

mqtt_parameter_8::mqtt_parameter_8(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_8::check_set(uint8_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_8::set(uint8_t input){
	set(input,true);
}
void mqtt_parameter_8::set(uint8_t input, bool update){
	_value = input;
	_update_required = update;
}
uint8_t mqtt_parameter_8::get_value(){
	return _value;
}
void mqtt_parameter_8::outdated(){
	outdated(true);
}
void mqtt_parameter_8::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_8::get_outdated(){
	return _update_required;
}


mqtt_parameter_16::mqtt_parameter_16(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_16::check_set(uint16_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_16::set(uint16_t input){
	set(input,true);
}
void mqtt_parameter_16::set(uint16_t input, bool update){
	_value = input;
	_update_required = update;
}
uint16_t mqtt_parameter_16::get_value(){
	return _value;
}
void mqtt_parameter_16::outdated(){
	outdated(true);
}
void mqtt_parameter_16::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_16::get_outdated(){
	return _update_required;
}


mqtt_parameter_32::mqtt_parameter_32(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_32::check_set(uint32_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_32::set(uint32_t input){
	set(input,true);
}
void mqtt_parameter_32::set(uint32_t input, bool update){
	_value = input;
	_update_required = update;
}
uint32_t mqtt_parameter_32::get_value(){
	return _value;
}
void mqtt_parameter_32::outdated(){
	outdated(true);
}
void mqtt_parameter_32::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_32::get_outdated(){
	return _update_required;
}
