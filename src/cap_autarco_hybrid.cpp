#include <cap_autarco_hybrid.h>
#ifdef WITH_AUTARCO_HYBRID
// simply the constructor
autarco_hybrid::autarco_hybrid(){
	m_discovery_pub = false;
	m_register_block = 0;
};

// simply the destructor
autarco_hybrid::~autarco_hybrid(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		// power
		char* t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_POWER_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Autarco power config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery power
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_POWER_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Autarco battery power config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// dc power
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC_POWER_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Autarco dc power config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// house power
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_HOUSE_POWER_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Autarco house power config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total battery
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total battery kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total battery discharge
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_DIS_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total battery discharge kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total from grid
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_FROM_GRID_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total from grid kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh total to grid
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_TO_GRID_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery total to grid kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily battery
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_BATTERY_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily battery kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily battery discharge
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_BATTERY_DIS_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily battery discharge kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily from grid
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_FROM_GRID_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily from grif kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// kwh daily to grid
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_TO_GRID_KWH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery daily to grid kwh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TEMP_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery temp"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 1 dc V
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC1_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC1V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 1 dc a
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC1_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC1A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 2 dc V
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC2_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC2V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 2 dc a
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC2_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC2A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 3 dc V
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC3_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC3V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// string 3 dc a
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC3_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery DC3A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery current
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_CURR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery A"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery current bms
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_CURR_BMS_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery A bms"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery voltage
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_VOLT_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery V"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery voltage BMS
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_VOLT_BMS_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery V bms"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery charge discharge
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_CHAR_DISCHAR_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery charge discharge"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery SOC
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_SOC_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery soc"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		// battery SOH
		t = discovery_topic_bake_2(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_SOH_TOPIC); // don't forget to "delete[] t;" at the end of usage;
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing Autarco discovery battery soh"), COLOR_GREEN);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
    
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("autarco deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just forwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool autarco_hybrid::parse(uint8_t * config){
	if(cap.parse(config, get_key())){
		return true;
	}
	return false;
}

// the will be requested to check if the key is in the config strim
uint8_t * autarco_hybrid::get_key(){
	return (uint8_t *) "AUTH";
}

// will be callen if the key is part of the config
bool autarco_hybrid::init(){
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
uint8_t autarco_hybrid::count_intervall_update(){
	return 1; 
}


void autarco_hybrid::build_crc(uint8_t in){
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
bool autarco_hybrid::loop(){
	while (swSer1->available()){
		m_timeout = 0;
		char char_in = swSer1->read();
		//logger.pln(char_in);

		// handle timeout .. reset to start after 10 msec silence 
		if(millis()-m_last_char_in>30 && m_parser_state!=AUTARCO_STATE_IDLE){
			m_parser_state = AUTARCO_STATE_HEAD;
		}
		m_last_char_in = millis();

		// state machine
		if(m_parser_state == AUTARCO_STATE_HEAD){
			if(char_in == 0x01){ // 0x01 = inverter addr
				m_parser_state = AUTARCO_STATE_CMD;
				m_crc = 0xFFFF; // reset, as this is start of message
				logger.pln("A");
			}
		} else if(m_parser_state == AUTARCO_STATE_CMD){
			if(char_in == 0x04){ // 0x04 = read register
				m_parser_state = AUTARCO_STATE_LEN;
				logger.pln("B");
			} else {
				m_parser_state = AUTARCO_STATE_IDLE;
			}
		} else if(m_parser_state == AUTARCO_STATE_LEN){
			if(char_in<AUTARCO_MAX_BUFFER){
				m_payload_len = char_in;
				m_payload_i = 0;
				logger.pln("C");
				m_parser_state = AUTARCO_STATE_PAYLOAD;
			} else {
				// not enough buffer space
				m_parser_state = AUTARCO_STATE_IDLE;
			}
		} else if(m_parser_state == AUTARCO_STATE_PAYLOAD){
			m_buffer[m_payload_i]= char_in;
			//logger.pln("D");
			m_payload_i++;
			if(m_payload_i == m_payload_len){
				m_parser_state = AUTARCO_STATE_CHECK_1;	
			}
		} else if(m_parser_state == AUTARCO_STATE_CHECK_1){
			m_parser_state = AUTARCO_STATE_CHECK_2;
			logger.pln("E");
		} else if(m_parser_state == AUTARCO_STATE_CHECK_2){
			//logger.println(TOPIC_MQTT_PUBLISH, F("check2"), COLOR_GREEN);
			build_crc(char_in);
			logger.pln("F");
			if(m_crc == 0x0000){
				logger.pln("G");
			//	logger.println(TOPIC_MQTT_PUBLISH, F("CRC 2 ok"), COLOR_GREEN);
			//}
			//if(1){
				if(m_req_start_addr == 33029){
					logger.pln("H");
					// All register are 16 bit, some data cover more that one register
					// 33000 | U16 |        | Product model, e.g. 05 0b
					// 33001 | U16 |        | DSP version, e.g. 05 0b
					// 33002 | U16 |        | HMI version, e.g. 05 0b
					// 33047 | U16 |        | AC output typ, e.g. 05 0b
					// 33048 | U16 |        | DC inputs, e.g. 0=1DC input,1=2DC input
					// 33057 | U32 |    W   | Total DC output power, e.g. 05 0b
					// 33029 | U32 |    kWh | Total Energy, e.g. 05 0b
					// 33031 | U32 |    kWh | Energy this month, e.g. 05 0b
					// 33033 | U32 |    kWh | Energy last month, e.g. 05 0b
					// 33035 | U16 | 0.1kWh | Energy today, e.g. 05 0b
					// 33036 | U16 | 0.1kWh | Energy last day, e.g. 05 0b
					// 33037 | U32 |    kWh | Energy this year, e.g. 05 0b
					// 33039 | U32 |    kWh | Energy last year, e.g. 05 0b
					// 33049 | U16 |  0.1V  | DC voltage 1, e.g. 05 0b
					// 33050 | U16 |  0.1A  | DC current 1, e.g. 05 0b
					// 33051 | U16 |  0.1V  | DC voltage 2, e.g. 05 0b
					// 33052 | U16 |  0.1A  | DC current 2, e.g. 05 0b
                    // 33053 | U16 |  0.1V  | DC voltage 3, e.g. 05 0b
					// 33054 | U16 |  0.1A  | DC current 3, e.g. 05 0b
                    // 33055 | U16 |  0.1V  | DC voltage 4, e.g. 05 0b
					// 33056 | U16 |  0.1A  | DC current 4, e.g. 05 0b
					// 33057 | U32 |  1W  | total dc output power, e.g. 05 0b
                    // 33079 | S32 |    W   | Active Power
                    // 33130 | S32 |   1W   | Meter active power
					// 33133 | U16 |  0.1V  | battery voltage
                    // 33134 | S16 |  0.1A  | battery current
                    // 33135 | U16 |  0.1A  | 0 charge 1 discharge
                    // 33139 | U16 |        | SOC 100 = 100%
                    // 33140 | U16 |        | SOH 100 = 100%
                    // 33141 | U16 |  0.1V  | battery voltage from BMS
                    // 33142 | S16 |  0.1A  | battery current from BMS
                    // 33147 | U16 |   1W   | household load
                    // 33148 | U16 |   1W   | backup load
                    // 33149 | S32 |   1W   | battery load
                    // 33151 | S32 |   1W   | Inverter AC grid port power
                    // 33161 | U32 |    kWh | Battery Total Charge Energy, e.g. 05 0b
					// 33163 | U16 | 0.1kWh | Today Battery charge energy
					// 33164 | U16 | 0.1kWh | yesterday Battery charge energy, e.g. 05 0b
					// 33165 | U32 |    kWh | Battery Total Disharge Energy
                    // 33167 | U16 | 0.1kWh | Today Battery discharge energy
					// 33168 | U16 | 0.1kWh | yesterday Battery discharge energy, e.g. 05 0b
                    // 33169 | U32 |    kWh | Total energy imported from grid
                    // 33171 | U16 | 0.1kWh | Energy from grid today, e.g. 05 0b
					// 33172 | U16 | 0.1kWh | Energy from grid last day, e.g. 05 0b
                    // 33173 | U32 |    kWh | Total energy fed to grid
                    // 33175 | U16 | 0.1kWh | total to grid today, e.g. 05 0b
					// 33176 | U16 | 0.1kWh | Energy to grid last day, e.g. 05 0b
                    // 33204 | U16 |        | battery charge discharge direction
                    // 33205 | U16 | 0.1A   | battery charge discharge current

					// active power
                    m_dc_power.set((m_buffer[(57-29)*2+2]<<8)+m_buffer[(57-29)*2+3]);
					m_dc1_volt.set(((float)((uint16_t)((m_buffer[(49-29)*2]<<8)+m_buffer[(49-29)*2+1])))/10);
					m_dc1_curr.set(((float)((uint16_t)((m_buffer[(50-29)*2]<<8)+m_buffer[(50-29)*2+1])))/10);
					m_dc2_volt.set(((float)((uint16_t)((m_buffer[(51-29)*2]<<8)+m_buffer[(51-29)*2+1])))/10);
					m_dc2_curr.set(((float)((uint16_t)((m_buffer[(52-29)*2]<<8)+m_buffer[(52-29)*2+1])))/10);
                    
                    
					// total kwh, total battery, total battery discharge, total from grid, total to grid
					// if m_last_total_kwh.get_value() == 0 set it and set it to not outdated, this is first time init update anyway
					if(m_last_total_kwh.get_value() == 0){ 
						m_last_total_kwh.set((m_buffer[(29-29)*2+0]<<24)+(m_buffer[(29-29)*2+1]<<16)+(m_buffer[(29-29)*2+2]<<8)+(m_buffer[(29-29)*2+3]<<0)); // 32 bit	
						m_last_total_kwh.outdated(false);
					} else {
						m_last_total_kwh.check_set((m_buffer[(29-29)*2+0]<<24)+(m_buffer[(29-29)*2+1]<<16)+(m_buffer[(29-29)*2+2]<<8)+(m_buffer[(29-29)*2+3]<<0)); // 32 bit
					};
					
                    

					// kwh today
					m_today_kwh.check_set(((float)((uint16_t)((m_buffer[(35-29)*2]<<8)+(m_buffer[(35-29)*2+1]))))/10); // float
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
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
  						m_total_kwh.set(m_last_total_kwh.get_value()+m_today_kwh.get_value()-m_kwh_sync);
                    };
                    
                    
                    

				} else if(m_req_start_addr == 33079){
					logger.pln("I");
					// All register are 16 bit, some data cover more that one register
					// 3042 | U16 |        | Product model, e.g. 05 0b
					m_power.set((m_buffer[(79-79)*2+0]<<24)+(m_buffer[(79-79)*2+1]<<16)+(m_buffer[(79-79)*2+2]<<8)+m_buffer[(79-79)*2+3]);
					m_temp.set(((float)((uint16_t)((m_buffer[(93-79)*2]<<8)+m_buffer[(93-79)*2+1])))/10);

				} else if(m_req_start_addr == 33130){
					logger.pln("J");
					m_meter_power.set(((m_buffer[(130-130)*2+0]<<24)+(m_buffer[(130-130)*2+1]<<16)+(m_buffer[(130-130)*2+2]<<8)+(m_buffer[(130-130)*2+3]))*(-1));
					m_bat_volt.set(((float)((uint16_t)((m_buffer[(133-130)*2]<<8)+m_buffer[(133-130)*2+1])))/10);
					logger.pln(m_bat_volt.get_value());
                    m_bat_curr.set(((float)((int16_t)((m_buffer[(134-130)*2]<<8)+m_buffer[(134-130)*2+1])))/10);
                    m_bat_volt_bms.set(((float)((uint16_t)((m_buffer[(141-130)*2]<<8)+m_buffer[(141-130)*2+1])))/100);
                    m_bat_curr_bms.set(((float)((int16_t)((m_buffer[(142-130)*2]<<8)+m_buffer[(142-130)*2+1])))/10);
                    //current charging or discharging
                    m_char_dischar.set((m_buffer[(135-130)*2+1]));
                    //SOC SOH
                    m_soc.set((m_buffer[(139-130)*2+1]));
                    m_soh.set((m_buffer[(140-130)*2+1]));
                    //SmartMeter data
                    m_house_load.set((m_buffer[(147-133)*2+0]<<8)+(m_buffer[(147-133)*2+1]));
                    m_battery_power.set((m_buffer[(149-133)*2+0]<<24)+(m_buffer[(149-133)*2+1]<<16)+(m_buffer[(149-133)*2+2]<<8)+m_buffer[(149-133)*2+3]);
                    m_ac_grid_port_power.set((m_buffer[(151-133)*2+0]<<24)+(m_buffer[(151-133)*2+1]<<16)+(m_buffer[(151-133)*2+2]<<8)+m_buffer[(151-133)*2+3]);
				} else if(m_req_start_addr == 33161){
					logger.pln("K");
					if(m_last_total_battery_kwh.get_value() == 0){ 
						m_last_total_battery_kwh.set((m_buffer[(161-161)*2+0]<<24)+(m_buffer[(161-161)*2+1]<<16)+(m_buffer[(161-161)*2+2]<<8)+(m_buffer[(161-161)*2+3]<<0)); // 32 bit	
						m_last_total_battery_kwh.outdated(false);
					} else {
						m_last_total_battery_kwh.check_set((m_buffer[(161-161)*2+0]<<24)+(m_buffer[(161-161)*2+1]<<16)+(m_buffer[(161-161)*2+2]<<8)+(m_buffer[(161-161)*2+3]<<0)); // 32 bit
					}
					if(m_last_total_battery_dis_kwh.get_value() == 0){ 
						m_last_total_battery_dis_kwh.set((m_buffer[(165-161)*2+0]<<24)+(m_buffer[(165-161)*2+1]<<16)+(m_buffer[(165-161)*2+2]<<8)+(m_buffer[(165-161)*2+3]<<0)); // 32 bit	
						m_last_total_battery_dis_kwh.outdated(false);
					} else {
						m_last_total_battery_dis_kwh.check_set((m_buffer[(165-161)*2+0]<<24)+(m_buffer[(165-161)*2+1]<<16)+(m_buffer[(165-161)*2+2]<<8)+(m_buffer[(165-161)*2+3]<<0)); // 32 bit
					}
                    if(m_last_total_from_grid_kwh.get_value() == 0){ 
						m_last_total_from_grid_kwh.set((m_buffer[(169-161)*2+0]<<24)+(m_buffer[(169-161)*2+1]<<16)+(m_buffer[(169-161)*2+2]<<8)+(m_buffer[(169-161)*2+3]<<0)); // 32 bit	
						m_last_total_from_grid_kwh.outdated(false);
					} else {
						m_last_total_from_grid_kwh.check_set((m_buffer[(169-161)*2+0]<<24)+(m_buffer[(169-161)*2+1]<<16)+(m_buffer[(169-161)*2+2]<<8)+(m_buffer[(169-161)*2+3]<<0)); // 32 bit
					}
					if(m_last_total_to_grid_kwh.get_value() == 0){ 
						m_last_total_to_grid_kwh.set((m_buffer[(173-161)*2+0]<<24)+(m_buffer[(173-161)*2+1]<<16)+(m_buffer[(173-161)*2+2]<<8)+(m_buffer[(173-161)*2+3]<<0)); // 32 bit	
						m_last_total_to_grid_kwh.outdated(false);
					} else {
						m_last_total_to_grid_kwh.check_set((m_buffer[(173-161)*2+0]<<24)+(m_buffer[(173-161)*2+1]<<16)+(m_buffer[(173-161)*2+2]<<8)+(m_buffer[(173-161)*2+3]<<0)); // 32 bit
					}
					m_today_battery_kwh.check_set(((float)((uint16_t)((m_buffer[(163-161)*2]<<8)+m_buffer[(163-161)*2+1])))/10); // float
					if(m_today_battery_kwh.get_outdated() && m_kwh_sync != 0.0){ // if today kWh (0.1 accuracy) updated, calc also total_kwh
						// so kwh today just jumped from e.g. 10.4 to 0.0
						// m_total_kwh was at 1234.5
						// m_last_total_kwh at 1234
						// so m_last_total+today_kwh-m_kwh_sync = 1234.5
						// 1234+0.0 - 0.0 - kwh_sync = 1234.5
						// - m_total_kwh + m_last_total_kwh = sync
						if(m_today_battery_kwh.get_value() == 0.0){
							// -0.5 = 1234 - 1234.5
							m_battery_kwh_sync = m_last_total_battery_kwh.get_value() - m_total_battery_kwh.get_value();
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
  						m_total_battery_kwh.set(m_last_total_battery_kwh.get_value()+m_today_battery_kwh.get_value()-m_battery_kwh_sync);
                    };
                    m_today_battery_dis_kwh.check_set(((float)((uint16_t)((m_buffer[(167-161)*2]<<8)+m_buffer[(167-161)*2+1])))/10); // float
					if(m_today_battery_dis_kwh.get_outdated() && m_kwh_sync != 0.0){ // if today kWh (0.1 accuracy) updated, calc also total_kwh
						// so kwh today just jumped from e.g. 10.4 to 0.0
						// m_total_kwh was at 1234.5
						// m_last_total_kwh at 1234
						// so m_last_total+today_kwh-m_kwh_sync = 1234.5
						// 1234+0.0 - 0.0 - kwh_sync = 1234.5
						// - m_total_kwh + m_last_total_kwh = sync
						if(m_today_battery_dis_kwh.get_value() == 0.0){
							// -0.5 = 1234 - 1234.5
							m_battery_dis_kwh_sync = m_last_total_battery_dis_kwh.get_value() - m_total_battery_dis_kwh.get_value();
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
  						m_total_battery_dis_kwh.set(m_last_total_battery_dis_kwh.get_value()+m_today_battery_dis_kwh.get_value()-m_battery_dis_kwh_sync);
                    };
                    m_today_from_grid_kwh.check_set(((float)((uint16_t)((m_buffer[(171-161)*2]<<8)+m_buffer[(171-161)*2+1])))/10); // float
					if(m_today_from_grid_kwh.get_outdated() && m_kwh_sync != 0.0){ // if today kWh (0.1 accuracy) updated, calc also total_kwh
						// so kwh today just jumped from e.g. 10.4 to 0.0
						// m_total_kwh was at 1234.5
						// m_last_total_kwh at 1234
						// so m_last_total+today_kwh-m_kwh_sync = 1234.5
						// 1234+0.0 - 0.0 - kwh_sync = 1234.5
						// - m_total_kwh + m_last_total_kwh = sync
						if(m_today_from_grid_kwh.get_value() == 0.0){
							// -0.5 = 1234 - 1234.5
							m_from_grid_kwh_sync = m_last_total_from_grid_kwh.get_value() - m_total_from_grid_kwh.get_value();
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
  						m_last_total_from_grid_kwh.set(m_last_total_from_grid_kwh.get_value()+m_today_from_grid_kwh.get_value()-m_from_grid_kwh_sync);
                    };    
                    m_today_to_grid_kwh.check_set(((float)((uint16_t)((m_buffer[(175-161)*2]<<8)+m_buffer[(175-161)*2+1])))/10); // float
					if(m_today_to_grid_kwh.get_outdated() && m_kwh_sync != 0.0){ // if today kWh (0.1 accuracy) updated, calc also total_kwh
						// so kwh today just jumped from e.g. 10.4 to 0.0
						// m_total_kwh was at 1234.5
						// m_last_total_kwh at 1234
						// so m_last_total+today_kwh-m_kwh_sync = 1234.5
						// 1234+0.0 - 0.0 - kwh_sync = 1234.5
						// - m_total_kwh + m_last_total_kwh = sync
						if(m_today_to_grid_kwh.get_value() == 0.0){
							// -0.5 = 1234 - 1234.5
							m_to_grid_kwh_sync = m_last_total_to_grid_kwh.get_value() - m_total_to_grid_kwh.get_value();
						}
						// 1234 + 0.0 - (-0.5) = 1234.5
  						m_last_total_to_grid_kwh.set(m_last_total_to_grid_kwh.get_value()+m_today_to_grid_kwh.get_value()-m_to_grid_kwh_sync);
  					};
					

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
		uint16_t len = 30;
		if(m_register_block==0){
			m_req_start_addr = 33029;
			len = 30;
		} else if(m_register_block==1){
			m_req_start_addr = 33079;
			len = 24;
		} else if(m_register_block==2){
			m_req_start_addr = 33130;
			len = 21;
		} else if(m_register_block==3){
			m_req_start_addr = 33161;
			len = 15;
			m_register_block = -1;
		} else {
			m_register_block = -1; // backup
		}
		m_register_block++;

		digitalWrite(RS485_DIR_PIN, HIGH); // speak
		m_msg_buffer[0] = 0x01; // inverter
		m_msg_buffer[1] = 0x04; // 0x04 = read register
		m_msg_buffer[2] = (m_req_start_addr - 0)>>8; //0x0B; // 3000-1=2999 = 0x0bb7
		m_msg_buffer[3] = (m_req_start_addr - 0) & 0xff; //0xB7; 
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
bool autarco_hybrid::intervall_update(uint8_t slot){
	// trigger send here?
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool autarco_hybrid::subscribe(){
	return true;
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool autarco_hybrid::publish(){
	
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			// RAM:   [======    ]  60.2% (used 49308 bytes from 81920 bytes)
			// Flash: [=====     ]  47.5% (used 454984 bytes from 958448 bytes)

			// current power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;   
			}
            // ac power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_AC_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery ac power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;   
			}
			// meter power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_METER_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery meter power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;   
			}
            //battery SOC
            if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_SOC_TOPIC,UNIT_NONE)){
                logger.println(TOPIC_MQTT_PUBLISH,F("Autarco Battery SOC"), COLOR_GREEN);
                	m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
            }
            //battery SOH
            if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_SOH_TOPIC,UNIT_NONE)){
                logger.println(TOPIC_MQTT_PUBLISH,F("Autarco Battery SOH"), COLOR_GREEN);
                	m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
            }
            // DC power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery dc power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // house power
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_HOUSE_POWER_TOPIC,UNIT_W)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery power"), COLOR_GREEN);
				m_discovery_pub = true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;    
			}

			// kwh total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}	
            // kwh battery total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total battery kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh battery discharge total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_DIS_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total battery dis kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh from grid total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_FROM_GRID_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh to grid total
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TOTAL_TO_GRID_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery total kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// kwh daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh battery daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_BATTERY_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily battery kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh battery discharge daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_BATTERY_DIS_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily battery discharge kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh from grid daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_FROM_GRID_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily from grid kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // kwh to grid daily
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DAY_TO_GRID_KWH_TOPIC,UNIT_KWH)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery daily to grid kwh"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// temperature
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_TEMP_TOPIC,UNIT_DEG_C)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery temp"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
			
			// string 1 dc V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC1_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC1V"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 1 dc A
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC1_CURR_TOPIC,UNIT_A)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC1A"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 2 dc V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC2_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC2V"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}

			// string 2 dc A
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC2_CURR_TOPIC,UNIT_A)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC2A"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
            }
             // string 3 dc V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_DC3_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery DC3V"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery V bms
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_VOLT_BMS_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery volt bms"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery V
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_VOLT_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery volt"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery A bms
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_CURR_BMS_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery curr bms"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery A
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_BATTERY_CURR_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco discovery curr"), COLOR_GREEN);
				m_discovery_pub &= true;
			} else {
				m_discovery_pub = false;
				return m_discovery_pub;
			}
            // battery charging or discharging
			if(discovery(DOMAIN_SENSOR,MQTT_AUTARCO_HYBRID_CHAR_DISCHAR_TOPIC,UNIT_V)){
				logger.println(TOPIC_MQTT_PUBLISH, F("Autarco charging discharching"), COLOR_GREEN);
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
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_power.outdated(false);
			return true;
		}
	}
    
    if(m_dc_power.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_dc_power.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DC_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc_power.outdated(false);
			return true;
		}
	}
    if(m_ac_grid_port_power.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_ac_grid_port_power.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco AC Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_AC_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_ac_grid_port_power.outdated(false);
			return true;
		}
	}
    
    if(m_battery_power.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_battery_power.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco Battery Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_BATTERY_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_battery_power.outdated(false);
			return true;
		}
	}
    
    if(m_house_load.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_house_load.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco house Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_HOUSE_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_house_load.outdated(false);
			return true;
		}
	}

    if(m_meter_power.get_outdated()){
		sprintf(m_msg_buffer,"%i",m_meter_power.get_value());
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco meter Power "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_METER_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_meter_power.outdated(false);
			return true;
		}
	}



	if(m_dc1_volt.get_outdated()){
		dtostrf(m_dc1_volt.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC1 Voltage "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DC1_VOLT_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc1_volt.outdated(false);
			return true;
		}
	}

	if(m_dc1_curr.get_outdated()){
		dtostrf(m_dc1_curr.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC1 Curr "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DC1_CURR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc1_curr.outdated(false);
			return true;
		}
	}

	if(m_dc2_volt.get_outdated()){
		dtostrf(m_dc2_volt.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC2 Voltage "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DC2_VOLT_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc2_volt.outdated(false);
			return true;
		}
	}

	if(m_dc2_curr.get_outdated()){
		dtostrf(m_dc2_curr.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco DC2 Curr "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DC2_CURR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_dc2_curr.outdated(false);
			return true;
		}
	}

	// today kwh
	if(m_today_kwh.get_outdated()){
		dtostrf(m_today_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DAY_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_kwh.outdated(false);
			return true;
		}
	}
    
    // today battery kwh
	if(m_today_battery_kwh.get_outdated()){
		dtostrf(m_today_battery_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DAY_BATTERY_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_battery_kwh.outdated(false);
			return true;
		}
	}
    
    // today battery discharge kwh
	if(m_today_battery_dis_kwh.get_outdated()){
		dtostrf(m_today_battery_dis_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today battery discharge kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DAY_BATTERY_DIS_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_battery_dis_kwh.outdated(false);
			return true;
		}
	}
    
    // today from grid kwh
	if(m_today_from_grid_kwh.get_outdated()){
		dtostrf(m_today_from_grid_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today from grid kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DAY_FROM_GRID_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_from_grid_kwh.outdated(false);
			return true;
		}
	}
    
    // today to grid kwh
	if(m_today_to_grid_kwh.get_outdated()){
		dtostrf(m_today_to_grid_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco today to grid kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_DAY_TO_GRID_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_today_to_grid_kwh.outdated(false);
			return true;
		}
	}

	// whenever m_last_total_kwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_kwh.get_outdated()){		
		m_kwh_sync = m_today_kwh.get_value();
		m_total_kwh.set(m_last_total_kwh.get_value());
		m_last_total_kwh.outdated(false);
	}
    
    // whenever m_last_total_battery_kwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_battery_kwh.get_outdated()){		
		m_battery_kwh_sync = m_today_battery_kwh.get_value();
		m_total_battery_kwh.set(m_last_total_battery_kwh.get_value());
		m_last_total_battery_kwh.outdated(false);
	}
    
    // whenever m_last_total_battery dis kwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_battery_dis_kwh.get_outdated()){		
		m_battery_dis_kwh_sync = m_today_battery_dis_kwh.get_value();
		m_total_battery_dis_kwh.set(m_last_total_battery_dis_kwh.get_value());
		m_last_total_battery_dis_kwh.outdated(false);
	}
    
    // whenever m_last_total_from gridkwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_from_grid_kwh.get_outdated()){		
		m_from_grid_kwh_sync = m_today_from_grid_kwh.get_value();
		m_total_from_grid_kwh.set(m_last_total_from_grid_kwh.get_value());
		m_last_total_from_grid_kwh.outdated(false);
	}
    
    // whenever m_last_total_to grid kwh changes, sync m_kwh_sync and set m_total_kwh hard, but there is noting to publish here
	if(m_last_total_to_grid_kwh.get_outdated()){		
		m_to_grid_kwh_sync = m_today_to_grid_kwh.get_value();
		m_total_to_grid_kwh.set(m_last_total_to_grid_kwh.get_value());
		m_last_total_to_grid_kwh.outdated(false);
	}
    

	// total kwh
	if(m_total_kwh.get_outdated()){
  		dtostrf(m_total_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TOTAL_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_kwh.outdated(false);
			return true;
		}
	}
    
    if(m_total_battery_kwh.get_outdated()){
  		dtostrf(m_total_battery_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total battery kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_battery_kwh.outdated(false);
			return true;
		}
	}
    
    if(m_total_battery_dis_kwh.get_outdated()){
  		dtostrf(m_total_battery_dis_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total battery discharge kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TOTAL_BATTERY_DIS_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_battery_dis_kwh.outdated(false);
			return true;
		}
	}
    
    if(m_total_from_grid_kwh.get_outdated()){
  		dtostrf(m_total_from_grid_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total from grid kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TOTAL_FROM_GRID_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_from_grid_kwh.outdated(false);
			return true;
		}
	}
    
    if(m_total_to_grid_kwh.get_outdated()){
  		dtostrf(m_total_to_grid_kwh.get_value(),8,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco total to grid kwh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TOTAL_TO_GRID_KWH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_total_to_grid_kwh.outdated(false);
			return true;
		}
	}
    // BATERRY SOC
	if(m_soc.get_outdated()){
  		dtostrf(m_soc.get_value(),6,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery soc "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_SOC_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_soc.outdated(false);
			return true;
		}
	}
    // BATERRY SOH
	if(m_soh.get_outdated()){
  		dtostrf(m_soh.get_value(),6,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery soh "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_SOH_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_soh.outdated(false);
			return true;
		}
    }
	// device temperature
	if(m_temp.get_outdated()){
  		dtostrf(m_temp.get_value(),6,1,m_msg_buffer); // 99 999 . 9 0
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco temp "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_TEMP_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_temp.outdated(false);
			return true;
		}
	}
    if(m_bat_curr_bms.get_outdated()){
		dtostrf(m_bat_curr_bms.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery bms a "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_BATTERY_CURR_BMS_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_bat_curr_bms.outdated(false);
			return true;
		}
	}
    if(m_bat_curr.get_outdated()){
		dtostrf(m_bat_curr.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery a "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_BATTERY_CURR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_bat_curr.outdated(false);
			return true;
		}
	}
    if(m_bat_volt_bms.get_outdated()){
		dtostrf(m_bat_volt_bms.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery bms V "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_BATTERY_VOLT_BMS_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_bat_volt_bms.outdated(false);
			return true;
		}
	}
    if(m_bat_volt.get_outdated()){
		dtostrf(m_bat_volt.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery V "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_BATTERY_VOLT_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_bat_volt.outdated(false);
			return true;
		}
	}
    if(m_char_dischar.get_outdated()){
		dtostrf(m_char_dischar.get_value(),4,1,m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Autraco battery charge discharge "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_AUTARCO_HYBRID_CHAR_DISCHAR_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_char_dischar.outdated(false);
			return true;
		}
	}

	return false;
}
#endif
