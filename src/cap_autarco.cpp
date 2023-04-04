#include <cap_autarco.h>
#ifdef WITH_AUTARCO
// simply the constructor
autarco::autarco(){
	m_discovery_pub = false;
};

// simply the destructor
autarco::~autarco(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		// power
		char* t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_POWER_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Autarco power config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_TOTAL_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_DAY_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_TEMP_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery temp"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 1 dc V
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_DC1_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC1V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 1 dc a
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_DC1_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC1A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 2 dc V
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_DC2_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC2V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 2 dc a
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_DC2_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC2A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;


		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("autarco deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool autarco::parse(uint8_t * config){
	if(cap.parse(config, get_key())){
		return true;
	}
	return false;
}

// the will be requested to check if the key is in the config strim
uint8_t * autarco::get_key(){
	return (uint8_t *) "AUT";
}

// will be callen if the key is part of the config
bool autarco::init(){
	logger.println(TOPIC_GENERIC_INFO, F("autarco init"), COLOR_GREEN);
	
	swSer1 = new SoftwareSerial(RS485_INPUT_PIN, RS485_OUPUT_PIN, false, 100); // 25 byte max buffer , 14 (D5) = rx,4 (D2) = tx
	swSer1->begin(9600);

	pinMode(RS485_DIR_PIN,OUTPUT);
	digitalWrite(RS485_DIR_PIN, LOW);

	m_parser_state = AUTARCO_STATE_IDLE;
	m_req_start_addr = 0x00;
	m_kwh_sync = 0;
	m_timeout = 0;
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t autarco::count_intervall_update(){
	return 1; 
}


void autarco::build_crc(uint8_t in){
	// based on https://github.com/LacobusVentura/MODBUS-CRC16
	static const uint16_t table[2] = { 0x0000, 0xA001 };
	unsigned int xorr = 0;
	m_crc ^= in;
	for(uint8_t bit = 0; bit < 8; bit++ ){
		xorr = m_crc & 0x01;
		m_crc >>= 1;
		m_crc ^= table[xorr];
	}
	return;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool autarco::loop(){
	while (swSer1->available()) {
		m_timeout = 0;
		char char_in = swSer1->read();
		//logger.pln(char_in);

		// handle timeout .. reset to start after 10 msec silence 
		if(millis()-m_last_char_in>10 && m_parser_state!=AUTARCO_STATE_IDLE){
			m_parser_state = AUTARCO_STATE_HEAD;
		}
		m_last_char_in = millis();

		// state machine
		if(m_parser_state == AUTARCO_STATE_HEAD){
			if(char_in == 0x01){ // 0x01 = inverter addr
				m_parser_state = AUTARCO_STATE_CMD;
				m_crc = 0xFFFF; // reset, as this is start of message
			}
		} else if(m_parser_state == AUTARCO_STATE_CMD){
			if(char_in == 0x04){ // 0x04 = read register
				m_parser_state = AUTARCO_STATE_LEN;
			} else {
				m_parser_state = AUTARCO_STATE_IDLE;
			}
		} else if(m_parser_state == AUTARCO_STATE_LEN){
			if(char_in<AUTARCO_MAX_BUFFER){
				m_payload_len = char_in;
				m_payload_i = 0;
				m_parser_state = AUTARCO_STATE_PAYLOAD;
			} else {
				// not enough buffer space
				m_parser_state = AUTARCO_STATE_IDLE;
			}
		} else if(m_parser_state == AUTARCO_STATE_PAYLOAD){
			m_buffer[m_payload_i]= char_in;
			m_payload_i++;
			if(m_payload_i == m_payload_len){
				m_parser_state = AUTARCO_STATE_CHECK_1;	
			}
		} else if(m_parser_state == AUTARCO_STATE_CHECK_1){
			m_parser_state = AUTARCO_STATE_CHECK_2;
		} else if(m_parser_state == AUTARCO_STATE_CHECK_2){
			//logger.println(TOPIC_MQTT_PUBLISH, F("check2"), COLOR_GREEN);
			build_crc(char_in);
			if(m_crc == 0x0000){
			//	logger.println(TOPIC_MQTT_PUBLISH, F("CRC 2 ok"), COLOR_GREEN);
			//}
			//if(1){
				if(m_req_start_addr == 3000){
					// All register are 16 bit, some data cover more that one register
					// 3000 | U16 |        | Product model, e.g. 05 0b
					// 3001 | U16 |        | DSP version, e.g. 05 0b
					// 3002 | U16 |        | LCD version, e.g. 05 0b
					// 3003 | U16 |        | AC output typ, e.g. 05 0b
					// 3004 | U16 |        | DC inputs, e.g. 0=1DC input,1=2DC input
					// 3005 | S32 |    W   | Active Power, e.g. 05 0b
					// 3007 | U32 |    W   | Total DC output power, e.g. 05 0b
					// 3009 | U32 |    kWh | Total Energy, e.g. 05 0b
					// 3011 | U32 |    kWh | Energy this month, e.g. 05 0b
					// 3013 | U32 |    kWh | Energy last month, e.g. 05 0b
					// 3015 | U16 | 0.1kWh | Energy total, e.g. 05 0b
					// 3016 | U16 | 0.1kWh | Energy last day, e.g. 05 0b
					// 3017 | U32 |    kWh | Energy this year, e.g. 05 0b
					// 3019 | U32 |    kWh | Energy last year, e.g. 05 0b
					// 3021 | U16 |        | HMI Version
					// 3022 | U16 |  0.1V  | DC voltage 1, e.g. 05 0b
					// 3023 | U16 |  0.1A  | DC current 1, e.g. 05 0b
					// 3024 | U16 |  0.1V  | DC voltage 2, e.g. 05 0b
					// 3025 | U16 |  0.1A  | DC current 2, e.g. 05 0b

					// active power
					m_power.set((m_buffer[5*2+2]<<8)+m_buffer[5*2+3]);
					m_dc1_volt.set(((float)((uint16_t)((m_buffer[22*2]<<8)+m_buffer[22*2+1])))/10);
					m_dc1_curr.set(((float)((uint16_t)((m_buffer[23*2]<<8)+m_buffer[23*2+1])))/10);
					m_dc2_volt.set(((float)((uint16_t)((m_buffer[24*2]<<8)+m_buffer[24*2+1])))/10);
					m_dc2_curr.set(((float)((uint16_t)((m_buffer[25*2]<<8)+m_buffer[25*2+1])))/10);

					// total kwh
					// if m_last_total_kwh.get_value() == 0 set it and set it to not outdated, this is first time init update anyway
					if(m_last_total_kwh.get_value() == 0){ 
						m_last_total_kwh.set((m_buffer[9*2+0]<<24)+(m_buffer[9*2+1]<<16)+(m_buffer[9*2+2]<<8)+(m_buffer[9*2+3]<<0)); // 32 bit	
						m_last_total_kwh.outdated(false);
					} else {
						m_last_total_kwh.check_set((m_buffer[9*2+0]<<24)+(m_buffer[9*2+1]<<16)+(m_buffer[9*2+2]<<8)+(m_buffer[9*2+3]<<0)); // 32 bit
					}

					// kwh today
					float last_today_kwh = m_today_kwh.get_value();
					m_today_kwh.check_set(((float)((uint16_t)((m_buffer[15*2]<<8)+m_buffer[15*2+1])))/10); // float
					if(m_today_kwh.get_outdated() && m_kwh_sync != 0.0){ // if today kWh (0.1 accuracy) updated, calc also total_kwh
						// so kwh today just jumped from e.g. 10.4 to 0.0
						// m_total_kwh was at 1234.5
						// m_last_total_kwh at 1234
						// so m_last_total+today_kwh-m_kwh_sync = 1234.5
						// 1234+0.0 - 0.0 - kwh_sync = 1234.5
						// - m_total_kwh + m_last_total_kwh = sync
						if(m_today_kwh.get_value() == 0.0){
							// -0.5 = 1234 - 1234.5
							m_kwh_sync = m_last_total_kwh.get_value() - m_total_kwh.get_value();
						} else if(last_today_kwh > 0.0 && m_today_kwh.get_value() == 0.1){ // sometimes the inverter jumps from e.g. 12.4 to 0.1 instead to 0.0
							m_kwh_sync = m_last_total_kwh.get_value() - m_total_kwh.get_value() - 0.1;
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
						m_total_kwh.set(m_last_total_kwh.get_value()+m_today_kwh.get_value()-m_kwh_sync);
					};

				} else if(m_req_start_addr == 3042){
					// All register are 16 bit, some data cover more that one register
					// 3042 | U16 |        | Product model, e.g. 05 0b
					m_temp.set(((float)((uint16_t)((m_buffer[0*2]<<8)+m_buffer[0*2+1])))/10);
				}
			//} else {
			//	logger.println(TOPIC_MQTT_PUBLISH, F("CRC 2 fail"), COLOR_RED);
			}
			m_parser_state = AUTARCO_STATE_IDLE;
		}
		// process message CRC, addr, length, payload ... but exclude CRC
		build_crc(char_in);
	}

	// retrigger
	if(millis()-m_last_char_in>7500){
		// after 10x 7.5 sec no response -> set current power to 0
		if(m_timeout<10){
			m_timeout++;
			if(m_timeout>=10){
				m_power.check_set(0);
			}
		}
		logger.println(TOPIC_MQTT_PUBLISH, F("Requesting new data from inverter"), COLOR_GREEN);
		m_last_char_in = millis();
		uint16_t len = 26;
		if(m_req_start_addr!=3000){
			m_req_start_addr = 3000;
			len = 26;
		} else {
			m_req_start_addr = 3042;
			len = 1;
		}
		digitalWrite(RS485_DIR_PIN, HIGH); // speak
		m_msg_buffer[0] = 0x01; // inverter
		m_msg_buffer[1] = 0x04; // 0x04 = read register
		m_msg_buffer[2] = (m_req_start_addr - 1)>>8; //0x0B; // 3000-1=2999 = 0x0bb7
		m_msg_buffer[3] = (m_req_start_addr - 1) & 0xff; //0xB7; 
		m_msg_buffer[4] = len >> 8; // length high
		m_msg_buffer[5] = len & 0xff; // length low

		m_crc = 0xFFFF;	// reset CRC calculation
		for(uint8_t i=0; i<6; i++){ // process message
			build_crc(m_msg_buffer[i]);
		}

		m_msg_buffer[6] = m_crc & 0xff; // CRC L
		m_msg_buffer[7] = (m_crc & 0xff00)>>8; // CRC H
		m_msg_buffer[8] = 0x00; // 0x00 why ever

		m_parser_state = AUTARCO_STATE_HEAD; // prepare statemachine to be ready to process answer
		swSer1->write(m_msg_buffer,9); // send message
		digitalWrite(RS485_DIR_PIN, LOW); // listen

	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool autarco::intervall_update(uint8_t slot){
	// trigger send here?
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool autarco::subscribe(){
	return true;
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool autarco::publish(){
	
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			// RAM:   [======    ]  60.2% (used 49308 bytes from 81920 bytes)
			// Flash: [=====     ]  47.5% (used 454984 bytes from 958448 bytes)

			// current power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// kwh total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_TOTAL_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}		

			// kwh daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_DAY_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// temperature
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_TEMP_TOPIC,UNIT_DEG_C)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery temp"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
			
			// string 1 dc V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_DC1_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC1V"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 1 dc A
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_DC1_CURR_TOPIC,UNIT_A)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC1A"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 2 dc V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_DC2_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC2V"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 2 dc A
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_DC2_CURR_TOPIC,UNIT_A)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC2A"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
		}
	}
#endif
	if(m_power.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_power.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_power.outdated(false);
			return true;
		}
	}


	if(m_dc1_volt.get_outdated()){
		dtostrf(m_dc1_volt.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC1 Voltage "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_DC1_VOLT_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc1_volt.outdated(false);
			return true;
		}
	}

	if(m_dc1_curr.get_outdated()){
		dtostrf(m_dc1_curr.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC1 Curr "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_DC1_CURR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc1_curr.outdated(false);
			return true;
		}
	}

	if(m_dc2_volt.get_outdated()){
		dtostrf(m_dc2_volt.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC2 Voltage "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_DC2_VOLT_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc2_volt.outdated(false);
			return true;
		}
	}

	if(m_dc2_curr.get_outdated()){
		dtostrf(m_dc2_curr.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC2 Curr "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_DC2_CURR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc2_curr.outdated(false);
			return true;
		}
	}

	// today kwh
	if(m_today_kwh.get_outdated()){
		dtostrf(m_today_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_DAY_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_kwh.outdated(false);
			return true;
		}
	}

	// whenever m_last_total_kwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_kwh.get_outdated()){		
		m_kwh_sync = m_today_kwh.get_value();
		m_total_kwh.set(m_last_total_kwh.get_value());
		m_last_total_kwh.outdated(false);
	}

	// total kwh
	if(m_total_kwh.get_outdated()){
  		dtostrf(m_total_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_TOTAL_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_kwh.outdated(false);
			return true;
		}
	}

	// device temperature
	if(m_temp.get_outdated()){
  		dtostrf(m_temp.get_value(),6,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco temp "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_TEMP_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_temp.outdated(false);
			return true;
		}
	}

	return false;
}
#endif
