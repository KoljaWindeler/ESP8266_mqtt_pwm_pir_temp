#include <cap_energy_meter_bin.h>
#ifdef WITH_EM_BIN
// simply the constructor
energy_meter_bin::energy_meter_bin(){
	m_discovery_pub = false;
	sprintf_P(m_total_grid_kwh,PSTR("no data"));
	sprintf_P(m_total_solar_kwh,PSTR("no data"));
	sprintf_P(m_current_w,PSTR("no data"));
	storage[0]=0;
	storage[1]=0;
};

// simply the destructor
energy_meter_bin::~energy_meter_bin(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_CUR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing EM cur config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing EM total grid config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing EM total solar config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("energy_meter_bin deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool energy_meter_bin::parse(uint8_t * config){
	if(cap.parse_wide(config,"%s%i",(uint8_t*)"BEM",1,100,&m_freq,(uint8_t*)"", true)){
		return true;
	} else if(cap.parse(config, get_key())){
		return true;
	}
	return false;
}

// the will be requested to check if the key is in the config strim
uint8_t * energy_meter_bin::get_key(){
	return (uint8_t *) "BEM";
}

// will be callen if the key is part of the config
bool energy_meter_bin::init(){
	logger.enable_serial_trace(false);
	logger.enable_mqtt_trace(true);
	Serial.flush();
	Serial.end();
	delay(50);
	Serial.begin(9600);
	logger.println(TOPIC_GENERIC_INFO, F("energy_meter_bin init"), COLOR_GREEN);
	pinMode(15,INPUT);
	//swSer1 = new SoftwareSerial(14, 4, false, 40); // 25 byte max buffer , 14 (D5) = rx,4 (D2) = tx
	//swSer1->begin(9600);
	dataset       = 0;
	m_parser_state = EM_STATE_WAIT;
	m_parser_i = 0;
	identifier[0] = (char *) ENERGY_METER_BIN_TOTAL_GRID_BIN;
	identifier[1] = (char *) ENERGY_METER_BIN_TOTAL_SOLAR_BIN;
	identifier[2] = (char *) ENERGY_METER_BIN_CUR_BIN;
	identifier_magic[0] = (char *) ENERGY_METER_BIN_TOTAL_GRID_BIN_MAGIC;
	identifier_magic[1] = (char *) ENERGY_METER_BIN_TOTAL_SOLAR_BIN_MAGIC;
	identifier_magic[2] = (char *) ENERGY_METER_BIN_CUR_BIN_MAGIC;
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t energy_meter_bin::count_intervall_update(){
	if(m_freq == 99){
		return 32; // 30x fast, total_solar, total_grid
	}
	return 2 * m_freq; // technically we're not publishing anything but we need to be called on a fixed time base
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed



bool energy_meter_bin::loop(){
	while (Serial.available()) {
		char char_in = Serial.read();
		//char_in &= 0x7F; // 8N1 to 7E1
		if(m_parser_state == EM_STATE_WAIT){
			if(identifier[dataset][m_parser_i] == char_in){
				m_parser_i++;
				if(m_parser_i == identifier_magic[dataset][0]){
					m_parser_state = EM_STATE_IGNORE;
					m_parser_ignore = identifier_magic[dataset][1];
				}
			} else {
				m_parser_i = 0;
			}
		} else if(m_parser_state == EM_STATE_IGNORE){
			m_parser_ignore--;
			if(m_parser_ignore==0){
				m_parser_state = EM_STATE_SCALE;
			}
		} else if(m_parser_state == EM_STATE_SCALE){
			if(char_in==0x00){
				m_parser_scale = 1;
			} else if(char_in==0x01){
				m_parser_scale = 10;
			} else if(char_in==0xFF){
				m_parser_scale = 0.1;
			} else if(char_in==0xFE){
				m_parser_scale = 0.01;
			} 
			m_parser_state = EM_STATE_IGNORE_2;
		} else if(m_parser_state == EM_STATE_IGNORE_2){
			m_parser_state = EM_STATE_PAYLOAD;
			m_parser_i = 0;
		} else if(m_parser_state == EM_STATE_PAYLOAD){
			temp[m_parser_i] = char_in;
			m_parser_i++;
			if(m_parser_i == identifier_magic[dataset][4]){
				m_parser_state = EM_STATE_CHECK;
			}
		} else if (m_parser_state == EM_STATE_CHECK){
			if(char_in == 0x01){
				if(dataset == 0 || dataset == 1){ // GRID / solar TOTAL
					float v=0;
					uint32_t v2=0;
					for(uint8_t i=0;i<identifier_magic[dataset][4];i++){
						v2 = (v2<<8)+(uint8_t)temp[i];
					}
					v=v2*m_parser_scale;
					v=v/1000; // wh -> kWh
					// limit increase to 10% (like way less, but this will filter spikes)
					if(v/storage[dataset]<1.1 or storage[dataset]==0){
						if(dataset == 0){ // grid
							dtostrf(v,0,4,m_total_grid_kwh);
						} else if(dataset == 1){ // solar
							dtostrf(v,0,4,m_total_solar_kwh);
						}
						storage[dataset] = v;
					} 
					// debug
					else {
						sprintf(m_msg_buffer,"Filtered value [%i]",dataset);
						logger.p(m_msg_buffer);
						dtostrf(v,0,4,m_msg_buffer);
						logger.pln(m_msg_buffer);
					}
					// debug
				} else if(dataset == 2){ // current value
					int32_t watt = (int) ((temp[0]<<24)+(temp[1]<<16)+(temp[2]<<8)+temp[3]);
					sprintf(m_current_w,"%i",watt);
				}
				dataset = (dataset+1)%3;
			}		
			m_parser_i = 0;
			m_parser_state = EM_STATE_WAIT;
		} else {
			m_parser_state = EM_STATE_WAIT;
		}
	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool energy_meter_bin::intervall_update(uint8_t slot){
	if(m_freq == 99){
		bool ret = 1;
		if(m_slot == 0){
			logger.print(TOPIC_MQTT_PUBLISH, F("EM Total Grid "), COLOR_GREEN);
			logger.pln(m_total_grid_kwh);
			ret = network.publish(build_topic(MQTT_ENERGY_METER_BIN_TOTAL_GRID_TOPIC, UNIT_TO_PC), m_total_grid_kwh);
		} else if(m_slot == 6){
			logger.print(TOPIC_MQTT_PUBLISH, F("EM Cur "), COLOR_GREEN);
			logger.pln(m_current_w);
			ret = network.publish(build_topic(MQTT_ENERGY_METER_BIN_CUR_TOPIC, UNIT_TO_PC), m_current_w);
		} else if(m_slot == 27){
			logger.print(TOPIC_MQTT_PUBLISH, F("EM Total Solar "), COLOR_GREEN);
			logger.pln(m_total_solar_kwh);
			ret = network.publish(build_topic(MQTT_ENERGY_METER_BIN_TOTAL_SOLAR_TOPIC,UNIT_TO_PC), m_total_solar_kwh);
		} else {
			logger.print(TOPIC_MQTT_PUBLISH, F("EM Cur "), COLOR_GREEN);
			logger.pln(m_current_w);
			ret = network.publish(build_topic(MQTT_ENERGY_METER_BIN_CUR_FAST_TOPIC, UNIT_TO_PC), m_current_w);
		}
		m_slot = (m_slot + 1) % 32;
		return ret;
	}
	
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool energy_meter_bin::subscribe(){
	return true;
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool energy_meter_bin::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_CUR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_EM_BIN_CUR_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_BIN_CUR_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM cur discovery Lall"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;

			t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_CUR_FAST_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EM_BIN_CUR_FAST_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_BIN_CUR_FAST_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM cur discovery L fast"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
		
			t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_BIN_TOTAL_GRID_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM total grid discovery"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;

			t = discovery_topic_bake(MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_BIN_TOTAL_SOLAR_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM total solar discovery"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif
	return false;
}
#endif