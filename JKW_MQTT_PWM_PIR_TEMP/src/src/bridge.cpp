#include <bridge.h>

// simply the constructor
bridge::bridge(){
	sprintf((char*)key,"RFB");
};

// simply the destructor
bridge::~bridge(){
	logger.println(TOPIC_GENERIC_INFO, F("Bridge deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool bridge::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* bridge::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool bridge::init(){
	logger.println(TOPIC_GENERIC_INFO, F("Bridge init"), COLOR_GREEN);
	Serial.end();
	Serial.begin(19200);
	m_state = RFB_START;
	rfb_data_rdy = false;
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t bridge::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool bridge::loop(){
	if(Serial.available()){
		if(!rfb_data_rdy){ // avoidloosing data
			t=Serial.read();
			//char temp[30];
			//sprintf(temp,"in: %02x state %i",t,m_state);
			//logger.pln(temp);
			rfb_last_received = millis();
			if(m_state == RFB_START){
				if(t == 0xAA){ // start byte
					m_state = RFB_ACTION;
				}
			} else if(m_state == RFB_ACTION){
				if(t == 0xA0){ // return
					m_state = RFB_END;
				} else if(t == 0xA2){ // learning timeout
					m_state = RFB_END;
				} else if(t == 0xA3){ // learning ok
					rfb_data[0]=t;
					m_state = RFB_SYNC_1;
				} else if(t == 0xA4){ // Receive
					rfb_data[0]=t;
					m_state = RFB_SYNC_1;
				}
			} else if(m_state >= RFB_SYNC_1 && m_state <= RFB_D2){	// gather all 9 byte
				rfb_data[1+m_state-RFB_SYNC_1]=t;
				if(m_state<RFB_D2){
					m_state++;
				} else {
					m_state = RFB_END;
				}
			} else if(m_state == RFB_END){ // check end marker
				if(t == 0x55){
					rfb_data_rdy = true;
				}
				m_state = RFB_START;
			}
		} else if((millis()-rfb_last_received)>RFB_TIMEOUT){
			m_state = RFB_START;
			rfb_data_rdy = false;
		}
	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool bridge::intervall_update(uint8_t slot){
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool bridge::subscribe(){
	network.subscribe(build_topic(MQTT_BRIDGE_TRAIN,PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_BRIDGE_TRAIN,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

void bridge::send_cmd(uint8_t cmd){
	char data[4];
	data[0]=0xAA;
	data[1]=cmd;
	data[2]=0x55;
	data[3]=0x00;
	Serial.print(data);
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool bridge::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_BRIDGE_TRAIN,PC_TO_UNIT))) {
		logger.println(TOPIC_GENERIC_INFO, F("Train seq received"), COLOR_GREEN);
		send_cmd(0xA1);
		return true;
	}
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool bridge::publish(){
	if(rfb_data_rdy){
		//publish something according to the data
		logger.print(TOPIC_MQTT_PUBLISH, F("incoming rf data "), COLOR_GREEN);
		sprintf(m_msg_buffer,"%02x%02x%02x",rfb_data[1+RFB_D0-RFB_SYNC_1],rfb_data[1+RFB_D1-RFB_SYNC_1],rfb_data[1+RFB_D2-RFB_SYNC_1]);
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_BRIDGE_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			rfb_data_rdy = false;
		}
		return true; // yes, we've published something, stop others (avoid overfilling mqtt buffer)
	}
	return false;
}
