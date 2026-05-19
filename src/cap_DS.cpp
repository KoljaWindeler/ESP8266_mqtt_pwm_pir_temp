#include <cap_DS.h>
#ifdef WITH_DS

J_DS::J_DS(){
	m_discovery_pub = false;
};

J_DS::~J_DS(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		undiscover(DOMAIN_SENSOR,MQTT_TEMPARATURE_COUNT_TOPIC);
		undiscover(DOMAIN_SENSOR,MQTT_TEMPARATURE_TOPIC);
		for(uint8_t i=0; i<m_sensor_count; i++){
			sprintf(m_msg_buffer,MQTT_TEMPARATURE_TOPIC_N,i+1);
			undiscover(DOMAIN_SENSOR,m_msg_buffer);
		};
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("DS deleted"), COLOR_YELLOW);
};

uint8_t* J_DS::get_key(){
	return (uint8_t*)"DS";
}


bool J_DS::init(){
	m_last_time_read = millis();
	p_ds = new OneWire(m_pin);
	m_sensor_count = 0;
	for(uint8_t i=0; i<DS_MAX_SENSORS; i++){
		if(p_ds->search(m_sensor_addr[i])) {
			// no more sensors on chain, reset search
			if (OneWire::crc8(m_sensor_addr[i], 7) != m_sensor_addr[i][7]) {
				logger.pln(F("CRC is not valid!"));
				i--;
			}
			else if (m_sensor_addr[i][0] != 0x10 && m_sensor_addr[i][0] != 0x28) {
				logger.p(F("Device is not recognized"));
				i--;
			} else {
				//for(uint8_t ii=0; ii<8; ii++){
				//	Serial.print(m_sensor_addr[i][ii],HEX);
				//}
				//Serial.println(" ");
				m_last_temp_published[i].set(DS_ERROR_TEMP);
				m_sensor_count++;
			}
			delay(1);
		} else {
			break;
		}
	}
	p_ds->reset_search();


	sprintf_P(m_msg_buffer, PSTR("DS init done, %i sensors found"), m_sensor_count);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
	m_discovery_pub=false;
	return true;
}

uint8_t J_DS::count_intervall_update(){
	return m_sensor_count; // we have 1 values; temp that we want to publish per minute
}

bool J_DS::intervall_update(uint8_t slot){
	// function called to publish the brightness of the led
	uint8_t sensor = slot % m_sensor_count;
	float temp = m_last_temp_published[sensor].get_value();
	if (temp==DS_ERROR_TEMP) {
		logger.print(TOPIC_GENERIC_INFO, F("no publish temp, "), COLOR_YELLOW);	
		dtostrf(temp, 3, 2, m_msg_buffer);
		logger.p(m_msg_buffer);
		logger.pln(F(" >TEMP_MAX"));
		return false;
	}
	logger.print(TOPIC_MQTT_PUBLISH, F(""));
	dtostrf(temp, 3, 2, m_msg_buffer);
	logger.p(F("DS temp "));
	logger.p(sensor);
	logger.p(" ");
	logger.pln(m_msg_buffer);
	if(sensor == 0){
		return network.publish(build_topic(MQTT_TEMPARATURE_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else {
		sprintf(m_topic_buffer, MQTT_TEMPARATURE_TOPIC_N, sensor+1);
		return network.publish(build_topic(m_topic_buffer,UNIT_TO_PC), m_msg_buffer);
	}
}



bool J_DS::parse(uint8_t* config){
	if(cap.parse(config,get_key())){
		m_pin = DS_PIN;
		return true;
	}
	return cap.parse_wide(config,get_key(),&m_pin);
}


bool J_DS::loop(){
	if(m_sensor_count > 0){
		if(millis()-m_last_time_read>(10000/m_sensor_count)){
			uint8_t this_sensor = (m_last_sensor_checked + 1) % m_sensor_count;
			float current_temp = getDsTemp(this_sensor);
			if (current_temp > TEMP_MAX || current_temp < (-1 * TEMP_MAX) || isnan(current_temp) || current_temp==DS_ERROR_TEMP ) {
				m_last_temp_published[this_sensor].set(DS_ERROR_TEMP);
			} else {
				float last_temp = m_last_temp_published[this_sensor].get_value();
				m_last_temp_published[this_sensor].set(current_temp);
				m_last_sensor_checked = this_sensor;
				if(fabs(last_temp-current_temp)<=0.5){ // abs works with int, so we scale everything by 10 to find 0.5 diff
					m_last_temp_published[this_sensor].outdated(false);
				}
			}
			m_last_time_read = millis();
			return true;
		}
	}
	return false;
}

bool J_DS::publish(){
	for(uint8_t i = 0; i < m_sensor_count; i++){
		if(m_last_temp_published[i].get_outdated()){
			if(intervall_update(i)){
				m_last_temp_published[i].outdated(false);
			}
			return true; // we've done network stuff, so we need to return true
		}
	}

#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			m_discovery_pub = true;
			if(m_sensor_count>0){
				for(uint8_t i=0; i<m_sensor_count; i++){
					wdt_reset();
					if(i==0){
						if(!discovery(DOMAIN_SENSOR,MQTT_TEMPARATURE_TOPIC,UNIT_DEG_C)){
							m_discovery_pub = false;
						}
					} else {
						sprintf(m_msg_buffer,MQTT_TEMPARATURE_TOPIC_N,i+1);
						if(!discovery(DOMAIN_SENSOR,m_msg_buffer,UNIT_DEG_C)){
							m_discovery_pub = false;
						}
					}
					delay(100);
				}
			}

			if(!discovery(DOMAIN_SENSOR,MQTT_TEMPARATURE_COUNT_TOPIC,UNIT_NONE)){
				m_discovery_pub = false;
			}

			sprintf(m_msg_buffer, "%i", m_sensor_count);
			network.publish(build_topic(MQTT_TEMPARATURE_COUNT_TOPIC,UNIT_TO_PC), m_msg_buffer);

			logger.println(TOPIC_MQTT_PUBLISH, F("DS discovery"), COLOR_GREEN);
			return true;
		}
	}
#endif
	return false;
}


float J_DS::getDsTemp(uint8_t sensor){ // https://blog.silvertech.at/arduino-temperatur-messen-mit-1wire-sensor-ds18s20ds18b20ds1820/
	// returns the temperature from one DS18S20 in DEG Celsius

	if(sensor >= m_sensor_count){
		return -1;
	}

	float TemperatureSum[5];
	byte data[12];

	for(uint8_t i=0; i<5; i++){	
		p_ds->reset();
		p_ds->select(m_sensor_addr[sensor]);
		p_ds->write(0x44, 1); // start conversion, with parasite power on at the end

		p_ds->reset();
		p_ds->select(m_sensor_addr[sensor]);
		p_ds->write(0xBE); // Read Scratchpad


		for (int i = 0; i < 9; i++) { // we need 9 bytes
			data[i] = p_ds->read();
		}

		p_ds->reset_search();

		byte MSB = data[1];
		byte LSB = data[0];

		float tempRead       = (int16_t) ((MSB << 8) | LSB); // using two's compliment
		TemperatureSum[i] = tempRead / 16.0;
	}

	// bubble sort 
	bool swap = true;
	float t;
	while(swap){
		swap = false;
		yield();
		for(uint8_t i=0; i<5-1; i++){
			if(TemperatureSum[i]>TemperatureSum[i+1]){
				t=TemperatureSum[i];
				TemperatureSum[i]=TemperatureSum[i+1];
				TemperatureSum[i]=t;
				swap=true;
				break;
			}
		}
	}
	
	return TemperatureSum[2]; // center of the sorted array
} // getDsTemp

uint8_t J_DS::getSensorCount(){
	return m_sensor_count;
}


// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool J_DS::subscribe(){
	for(uint8_t i=1; i<=m_sensor_count; i++){
		wdt_reset();
		// set J_GPIO state
		if(i>1){
			sprintf(m_msg_buffer, MQTT_TEMPARATURE_TOPIC_N, i);
		} else {
			sprintf(m_msg_buffer, MQTT_TEMPARATURE_TOPIC);
		}
		network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);
		delay(100);
	}

	// ID subscribe
	sprintf(m_msg_buffer, "%s_%s", MQTT_TEMPARATURE_TOPIC,"id");
	network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool J_DS::receive(uint8_t* p_topic, uint8_t* p_payload){
	// //////////////// MASS DIMMING //////////////////
	// MASS dimm pins .. e.g.  "gpio_255_dimm" >> "ON,13,15"
	for(uint8_t i=1; i<=m_sensor_count; i++){
		wdt_reset();
		// set J_GPIO state
		if(i>1){
			sprintf(m_msg_buffer, MQTT_TEMPARATURE_TOPIC_N, i);
		} else {
			sprintf(m_msg_buffer, MQTT_TEMPARATURE_TOPIC);
		}
		if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
			intervall_update(i-1);
			return true;
		}
	}

	// post IDs
	sprintf(m_msg_buffer, "%s_%s", MQTT_TEMPARATURE_TOPIC,"id");
	if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
		for(uint8_t i=0; i<m_sensor_count; i++){
			for(uint8_t ii=0; ii<8; ii++){
				sprintf(m_msg_buffer+2*ii,"%02x", m_sensor_addr[i][ii]); // 8 byte
			}
			m_msg_buffer[16]=0x00;
			sprintf((char*)p_topic, "%s_%s_%i", MQTT_TEMPARATURE_TOPIC,"id",i);
			network.publish(build_topic((const char*)p_topic,UNIT_TO_PC), m_msg_buffer);
		}
		return true;
	}

	return false;
}


#endif
