#ifdef WITH_EBUS
#include <cap_ebus.h>

// simply the constructor
ebus::ebus(){
	m_txPin = 4; // D2
	m_rxPin = 5; // D1
	m_discovery_pub=false;
};

// simply the destructor
ebus::~ebus(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_HEATER_SET_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ebus heater set config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		//
		t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_WW_SET_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ebus ww set config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		//
		t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_HEATER_REQ_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ebus heater req config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		//
		t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_ROOM_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ebus room config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("ebus deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool ebus::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* ebus::get_key(){
	return (uint8_t*)"EBUS";
}

// will be callen if the key is part of the config
bool ebus::init(){
	// datasets
	identifier[0] = (char *) EBUS_HEATER_SET;
	identifier[1] = (char *) EBUS_WW_SET;
	identifier[2] = (char *) EBUS_HEATER_REQ_SET;
	identifier[3] = (char *) EBUS_ROOM;
	identifier_magic[0] = (uint16_t *) EBUS_HEATER_SET_MAGIC;
	identifier_magic[1] = (uint16_t *) EBUS_WW_SET_MAGIC;
	identifier_magic[2] = (uint16_t *) EBUS_HEATER_REQ_SET_MAGIC;
	identifier_magic[3] = (uint16_t *) EBUS_ROOM_MAGIC;
	
	// state variables
	m_dataset = 0;
	m_parser_state = EBUS_STATE_HEADER;
	m_parser_data = 0;
	m_parser_ignore = 0;
	
	// serial interface
	swSer1 = new SoftwareSerial(m_rxPin, m_txPin, true);
	swSer1->begin(2400);
	swSer1->enableIntTx(true);
	logger.println(TOPIC_GENERIC_INFO, F("ebus init"), COLOR_GREEN);
	return true;
}

// override-methode, only implement when needed
// e.g. the light class needs a provider to actually do something, so it implements this
//bool play::reg_provider(peripheral * p, uint8_t* t){
//	return false;
//}

// override-methode, only implement when needed
// return the key of any component that you depend on, e.g. "PWM" depends on "LIG"
//uint8_t* play::get_dep(){
//	return (uint8_t*)"";
//}

// override-methode, only implement when needed
// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t ebus::count_intervall_update(){
	return 0;
}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool ebus::loop(){
	uint8_t buf;
	while(swSer1->available()){
		buf=swSer1->read();

		if(m_parser_state == EBUS_STATE_HEADER){
			if(buf == identifier[m_dataset][m_parser_data]){
				m_parser_data++;
				if(m_parser_data == identifier_magic[m_dataset][0]){
					m_parser_state = EBUS_STATE_IGNORE;
					m_parser_ignore = 0;
				}
			} else { 
				m_parser_data = 0;
			}
		} else if(m_parser_state == EBUS_STATE_IGNORE){
			m_parser_ignore++;
			if(m_parser_ignore == identifier_magic[m_dataset][1]){
				m_parser_state = EBUS_STATE_PAYLOAD;
				m_parser_data = 0;
			}
		} else if(m_parser_state == EBUS_STATE_PAYLOAD){
			m_parser_data++;
			uint16_t v;
			if(identifier_magic[m_dataset][2]==2){ // 16 bit value, LSB first
				if(m_parser_data==1){
					m_temp_buffer = buf;
				} else {
					v = buf << 8;
					v += m_temp_buffer;
				}
			} else { // 8 bit value
				v = buf;
			}
			if(identifier_magic[m_dataset][2] == m_parser_data){
				// save according to dataset
				if(m_dataset==0){ // Heater SET 
					m_state_heater_set.set((float)v/(float)identifier_magic[m_dataset][3]);
				} else if(m_dataset==1){ // WW set 				
					m_state_ww_set.set((float)v/(float)identifier_magic[m_dataset][3]);
				} else if(m_dataset==2){ // Heater request 
					m_state_heater_req_set.set((float)v/(float)identifier_magic[m_dataset][3]);
				} else if(m_dataset==3){ // Room Temp
					m_state_room.set((float)v/(float)identifier_magic[m_dataset][3]);
				}
				// reset parser
				m_dataset = (m_dataset + 1)%4; // 0,1,2,3,0,1,2,3 ..
				m_parser_data = 0;
				m_parser_state = EBUS_STATE_HEADER;
			}
		} else {
			m_parser_state = EBUS_STATE_HEADER;
		}	
		yield();
	}
	return false; // i did nothing
}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool ebus::intervall_update(uint8_t slot){
	return false;
}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool ebus::subscribe(){
//	network.subscribe(build_topic(MQTT_ebus_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_ebus_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool ebus::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_ebus_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received ebus command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool ebus::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t;
			char* m;
			//
			delay(200);
			t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_HEATER_SET_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EBUS_HEATER_SET_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EBUS_HEATER_SET_MSG, mqtt.dev_short, mqtt.dev_short);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			yield();
			logger.println(TOPIC_MQTT_PUBLISH, F("ebus heater set discovery"), COLOR_GREEN);
			//
			delay(200);
			t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_WW_SET_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EBUS_WW_SET_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EBUS_WW_SET_MSG, mqtt.dev_short, mqtt.dev_short);
			m_discovery_pub &= network.publish(t,m);
			delete[] m;
			delete[] t;
			yield();
			logger.println(TOPIC_MQTT_PUBLISH, F("ebus ww set discovery"), COLOR_GREEN);
			//
			delay(200);
			t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_HEATER_REQ_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EBUS_HEATER_REQ_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EBUS_HEATER_REQ_MSG, mqtt.dev_short, mqtt.dev_short);
			m_discovery_pub &= network.publish(t,m);
			delete[] m;
			delete[] t;
			yield();
			logger.println(TOPIC_MQTT_PUBLISH, F("ebus heater req discovery"), COLOR_GREEN);
			//
			delay(200);
			t = discovery_topic_bake(MQTT_DISCOVERY_EBUS_ROOM_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EBUS_ROOM_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EBUS_ROOM_MSG, mqtt.dev_short, mqtt.dev_short);
			m_discovery_pub &= network.publish(t,m);
			delete[] m;
			delete[] t;
			yield();
			logger.println(TOPIC_MQTT_PUBLISH, F("ebus room discovery"), COLOR_GREEN);
			return true;
		}
	}
#endif

	boolean ret = false;
	if (m_state_heater_set.get_outdated()) {
		dtostrf(m_state_heater_set.get_value(), 3, 2, m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("FBH set temp state "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_HEATER_SET_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_state_heater_set.outdated(false);
		}
		return ret;
	} else if (m_state_ww_set.get_outdated()) {
		dtostrf(m_state_ww_set.get_value(), 3, 2, m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("WW set temp state "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_WW_SET_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_state_ww_set.outdated(false);
		}
		return ret;
	} else if (m_state_heater_req_set.get_outdated()) {
		dtostrf(m_state_heater_req_set.get_value(), 3, 2, m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("Heater requested state "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_HEATER_REQ_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_state_heater_req_set.outdated(false);
		}
		return ret;
	} else if (m_state_room.get_outdated()) {
		dtostrf(m_state_room.get_value(), 3, 2, m_msg_buffer);
		logger.print(TOPIC_MQTT_PUBLISH, F("room temp state "), COLOR_GREEN);
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_ROOM_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_state_room.outdated(false);
		}
		return ret;
	}
	return false;
}
#endif
