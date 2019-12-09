#include <cap_energy_meter.h>

// simply the constructor
energy_meter::energy_meter(){
	m_discovery_pub = false;
};

// simply the destructor
energy_meter::~energy_meter(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_EM_CUR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing EM cur config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		t = discovery_topic_bake(MQTT_DISCOVERY_EM_TOTAL_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing EM total config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("energy_meter deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool energy_meter::parse(uint8_t * config){
	return cap.parse(config, get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t * energy_meter::get_key(){
	return (uint8_t *) "EM";
}

// will be callen if the key is part of the config
bool energy_meter::init(){
	logger.println(TOPIC_GENERIC_INFO, F("energy_meter init"), COLOR_GREEN);
	swSer1 = new SoftwareSerial(14, 4, false, 40); // 25 byte max buffer , 14 (D5) = rx,4 (D2) = tx
	swSer1->begin(9600);
	dataset       = 0;
	identifier[0] = (char *) ENERGY_METER_TOTAL;
	identifier[1] = (char *) ENERGY_METER_CUR;
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t energy_meter::count_intervall_update(){
	return 2; // technically we're not publishing anything but we need to be called on a fixed time base
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool energy_meter::loop(){
	while (swSer1->available()) {
		char char_in = swSer1->read();
		char_in &= 0x7F; // 8N1 to 7E1
		//Serial.printf("Recv char %c\r\n",char_in);
		if (m_state.get_value() < strlen(identifier[dataset])) {
			//Serial.printf("section 1 %c\r\n",identifier[dataset][m_state.get_value()]);
			if (identifier[dataset][m_state.get_value()] == char_in) {
				m_state.set(m_state.get_value() + 1);
			} else {
				m_state.set(0);
			}
		} else if (char_in == '*') {
			//Serial.printf("section 3: %i\r\n",temp);
			// clean leading 0
			for(uint8_t i=0;temp[i]=='0';i++){
				temp[i]=' ';
			}

			if (dataset == ENEGERY_METER_TOTAL) {
				memcpy(m_total_kwh,temp,15);
				m_total_kwh[15]=0x00;
				//Serial.printf("found %s\n",m_total_kwh);
			} else {
				memcpy(m_current_w,temp,9);
				m_current_w[9]=0x00;
				//Serial.printf("found %s\n",m_current_w);
			}
			m_state.set(0);
			dataset = !dataset;
		} else {
			temp[m_state.get_value() - strlen(identifier[dataset])] = char_in;
			m_state.set(m_state.get_value() + 1);
		}
	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool energy_meter::intervall_update(uint8_t slot){
	if (slot % 2 == 0) {
		logger.print(TOPIC_MQTT_PUBLISH, F("EM Total "), COLOR_GREEN);
		logger.pln(m_total_kwh);
		return network.publish(build_topic(MQTT_ENERGY_METER_TOTAL_TOPIC, UNIT_TO_PC), m_total_kwh);
	} else {
		logger.print(TOPIC_MQTT_PUBLISH, F("EM Cur "), COLOR_GREEN);
		logger.pln(m_current_w);
		return network.publish(build_topic(MQTT_ENERGY_METER_CUR_TOPIC, UNIT_TO_PC), m_current_w);
	}
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool energy_meter::subscribe(){
	return true;
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool energy_meter::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_EM_CUR_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_EM_CUR_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_CUR_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM cur discovery"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			t = discovery_topic_bake(MQTT_DISCOVERY_EM_TOTAL_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			m = new char[strlen(MQTT_DISCOVERY_EM_TOTAL_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_EM_TOTAL_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("EM total discovery"), COLOR_GREEN);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif
	return false;
}
