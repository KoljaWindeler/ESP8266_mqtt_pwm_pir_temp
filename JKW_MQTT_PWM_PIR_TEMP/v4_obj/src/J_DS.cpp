#include <J_DS.h>

OneWire ds(DS_PIN);

J_DS::J_DS(){};
J_DS::~J_DS(){
	logger.println(TOPIC_GENERIC_INFO, F("DS deleted"), COLOR_YELLOW);
};
void J_DS::interrupt(){};


uint8_t* J_DS::get_key(){
	sprintf((char*)key,"DS");
	return key;
}

bool J_DS::init(){
	logger.println(TOPIC_GENERIC_INFO, F("DS init"), COLOR_GREEN);
	return true;
}

uint8_t J_DS::count_intervall_update(){
	return 1; // we have 1 values; temp that we want to publish per minute
}

bool J_DS::loop(){
	return false; // i did nothing
}

bool J_DS::intervall_update(uint8_t slot){
	// function called to publish the brightness of the led
	float temp = getDsTemp();
	if (temp > TEMP_MAX || temp < (-1 * TEMP_MAX) || isnan(temp)) {
		logger.print(TOPIC_MQTT, F("no publish temp, "), COLOR_YELLOW);
		if (isnan(temp)) {
			Serial.println(F("nan"));
		} else {
			Serial.print(temp);
			Serial.println(F(" >TEMP_MAX"));
		}
		return false;
	}

	logger.print(TOPIC_MQTT_PUBLISH, F(""));
	dtostrf(temp, 3, 2, m_msg_buffer);
	Serial.print(F("DS temp "));
	Serial.println(m_msg_buffer);
	return client.publish(build_topic(MQTT_TEMPARATURE_TOPIC), m_msg_buffer, true);
}

bool J_DS::subscribe(){
	return true;
}

bool J_DS::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

bool J_DS::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool J_DS::publish(){
	return false;
}


float J_DS::getDsTemp(){ // https://blog.silvertech.at/arduino-temperatur-messen-mit-1wire-sensor-ds18s20ds18b20ds1820/
	// returns the temperature from one DS18S20 in DEG Celsius

	byte data[12];
	byte addr[8];

	if (!ds.search(addr)) {
		// no more sensors on chain, reset search
		ds.reset_search();
		return -999;
	}

	if (OneWire::crc8(addr, 7) != addr[7]) {
		Serial.println(F("CRC is not valid!"));
		return -888;
	}

	if (addr[0] != 0x10 && addr[0] != 0x28) {
		Serial.print(F("Device is not recognized"));
		return -777;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1); // start conversion, with parasite power on at the end

	byte present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad


	for (int i = 0; i < 9; i++) { // we need 9 bytes
		data[i] = ds.read();
	}

	ds.reset_search();

	byte MSB = data[1];
	byte LSB = data[0];

	float tempRead       = ((MSB << 8) | LSB); // using two's compliment
	float TemperatureSum = tempRead / 16;

	return TemperatureSum;
} // getDsTemp
