#include <cap_DS.h>
#ifdef WITH_DS

J_DS::J_DS(){
	m_discovery_pub = false;
};

J_DS::~J_DS(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_DS_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing DS config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("DS deleted"), COLOR_YELLOW);
};

uint8_t* J_DS::get_key(){
	return (uint8_t*)"DS";
}

bool J_DS::init(){
	p_ds = new OneWire(m_pin);
	logger.println(TOPIC_GENERIC_INFO, F("DS init"), COLOR_GREEN);
	m_discovery_pub=false;
	return true;
}

uint8_t J_DS::count_intervall_update(){
	return 1; // we have 1 values; temp that we want to publish per minute
}

bool J_DS::intervall_update(uint8_t slot){
	// function called to publish the brightness of the led
	float temp = getDsTemp();
	if (temp > TEMP_MAX || temp < (-1 * TEMP_MAX) || isnan(temp)) {
		logger.print(TOPIC_GENERIC_INFO, F("no publish temp, "), COLOR_YELLOW);
		if (isnan(temp)) {
			logger.pln(F("nan"));
		} else {
			dtostrf(temp, 3, 2, m_msg_buffer);
			logger.p(m_msg_buffer);
			logger.pln(F(" >TEMP_MAX"));
		}
		return false;
	}

	logger.print(TOPIC_MQTT_PUBLISH, F(""));
	dtostrf(temp, 3, 2, m_msg_buffer);
	logger.p(F("DS temp "));
	logger.pln(m_msg_buffer);
	return network.publish(build_topic(MQTT_TEMPARATURE_TOPIC,UNIT_TO_PC), m_msg_buffer);
}



bool J_DS::parse(uint8_t* config){
	if(cap.parse(config,get_key())){
		m_pin = DS_PIN;
		return true;
	}
	return cap.parse_wide(config,get_key(),&m_pin);
}

bool J_DS::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

bool J_DS::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_DS_TOPIC); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_DS_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_DS_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("DS discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif
	return false;
}


float J_DS::getDsTemp(){ // https://blog.silvertech.at/arduino-temperatur-messen-mit-1wire-sensor-ds18s20ds18b20ds1820/
	// returns the temperature from one DS18S20 in DEG Celsius

	byte data[12];
	byte addr[8];

	if (!p_ds->search(addr)) {
		// no more sensors on chain, reset search
		p_ds->reset_search();
		return -999;
	}

	if (OneWire::crc8(addr, 7) != addr[7]) {
		logger.pln(F("CRC is not valid!"));
		return -888;
	}

	if (addr[0] != 0x10 && addr[0] != 0x28) {
		logger.p(F("Device is not recognized"));
		return -777;
	}

	p_ds->reset();
	p_ds->select(addr);
	p_ds->write(0x44, 1); // start conversion, with parasite power on at the end

	p_ds->reset();
	p_ds->select(addr);
	p_ds->write(0xBE); // Read Scratchpad


	for (int i = 0; i < 9; i++) { // we need 9 bytes
		data[i] = p_ds->read();
	}

	p_ds->reset_search();

	byte MSB = data[1];
	byte LSB = data[0];

	float tempRead       = ((MSB << 8) | LSB); // using two's compliment
	float TemperatureSum = tempRead / 16;

	return TemperatureSum;
} // getDsTemp
#endif
