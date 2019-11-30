#include <cap_tuya_bridge.h>
#ifdef WITH_TUYA_BRIDGE
// simply the constructor
tuya_bridge::tuya_bridge(){};

// simply the destructor
tuya_bridge::~tuya_bridge(){
	logger.println(TOPIC_GENERIC_INFO, F("tuya_bridge deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool tuya_bridge::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* tuya_bridge::get_key(){
	return (uint8_t*)"RFB";
}

// will be callen if the key is part of the config
bool tuya_bridge::init(){
	logger.enable_mqtt_trace(true);
	logger.println(TOPIC_GENERIC_INFO, F("tuya_bridge init"), COLOR_GREEN);
	logger.enable_serial_trace(false);
	Serial.end();
	delay(50);
	Serial.begin(9600);

	// ready to listen
	m_recv_state = RFB_START_0;
	m_msg_state = 0;

	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t tuya_bridge::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool tuya_bridge::loop(){
	if(m_msg_state%2==0){ // ich bin
		if(m_msg_state==0){
			send_cmd(1, 0, (uint8_t*)"");// request generic info
			logger.println(TOPIC_GENERIC_INFO,"generic request send",COLOR_GREEN);
			m_msg_state=5; // wait for reply
			//m_msg_state++; // wait for reply, skip the two steps in between, not needed i think
		}

		else if(m_msg_state==2){
			//uint8_t test3[8]={0x55,0xAA,0x00,0x02,0x00,0x01,0x02,0x04};
			//Serial.write(test3,8);
			uint8_t test3 = 0x02;
			send_cmd(2, 1, &test3);// request generic info
			logger.println(TOPIC_GENERIC_INFO,"request 212 send",COLOR_GREEN);
			rfb_last_received=millis();
			m_msg_state++; // wait for reply
		}

		else if(m_msg_state==4){
			//uint8_t test4[8]={0x55,0xAA,0x00,0x02,0x00,0x01,0x02,0x04};
			//Serial.write(test4,8);
			uint8_t test3 = 0x03;
			send_cmd(2, 1, &test3);// request generic info
			logger.println(TOPIC_GENERIC_INFO,"request 213 send",COLOR_GREEN);
			rfb_last_received=millis();
			m_msg_state++; // wait for reply
		}

		else if(m_msg_state==6){
			uint8_t a=0x04;
			send_cmd(2, 1, &a);
			logger.println(TOPIC_GENERIC_INFO,"status 214 send",COLOR_GREEN);
			m_msg_state++; // wait for status
		}
	}

	// state machine to parse incoming data
	if(Serial.available()){
		t=Serial.read();
		rfb_last_received = millis();
		if(m_recv_state == RFB_START_0){
			if(t == 0x55){ // start byte
				//logger.pln("s");
				rfb.chk=t;
				m_recv_state++;
			}
		} else if(m_recv_state == RFB_START_1){
			if(t == 0xAA){ // start byte
				//logger.pln("st1");
				rfb.chk+=t;
				m_recv_state++;
			} else {
				m_recv_state = RFB_START_0;
			}
		} else if(m_recv_state == RFB_VERSION){
			//logger.pln("v");
			m_recv_state++;
			rfb.chk+=t;
		} else if(m_recv_state == RFB_CMD){
			//logger.pln("cmd");
			m_recv_state++;
			rfb.cmd=t;
			rfb.len=0;
			rfb.chk+=t;
		} else if(m_recv_state == RFB_LEN_1 || m_recv_state == RFB_LEN_2){
			//logger.pln("len");
			rfb.len=(rfb.len<<8)+t;
			rfb.chk+=t;
			rfb.data_p=0;
			if(m_recv_state == RFB_LEN_2 && rfb.len==0){
				m_recv_state=RFB_CHK;
			} else {
				m_recv_state++;
			}
		} else if(m_recv_state == RFB_DATA){
			//logger.p("d");
			rfb.chk+=t;
			rfb.data[rfb.data_p]=t;
			rfb.data_p++;
			if(rfb.data_p==rfb.len){
				m_recv_state++;
			}
		} else if(m_recv_state == RFB_CHK){
			logger.pln("chk");
			if(rfb.chk==t){
				logger.pln("ok");
				//char m[80];
				//sprintf(m,"len: %i, cmd: %i, [chk]:%x",rfb.len,rfb.cmd,rfb.chk);
				if(rfb.cmd==5 && rfb.len>=18){
					m_battery.set(rfb.data[17]);
					//status.tamper = rfb.data[9];
					m_state.set(rfb.data[4]);
					//rfb.rdy=true;
				}
				//logger.pln(m);
				m_msg_state++;
			}
			m_recv_state = RFB_START_0;
		}
	} // rfb.rdy while

	/*if(millis()-rfb_last_received > 200 && m_msg_state>0){
		rfb_last_received=millis();
		m_msg_state--;
	}*/
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool tuya_bridge::intervall_update(uint8_t slot){
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool tuya_bridge::subscribe(){
	// for this component we're sending out a wakeup messages as early as possible
	// right after we're connected the subscribe will be callen, even before the first
	// loop, so send it right away
	sprintf(m_msg_buffer,"Tuya Wake Up");
	logger.println(TOPIC_MQTT_PUBLISH, F("Tuya Wake Up"), COLOR_GREEN);
	network.publish(build_topic(MQTT_TUYA_BRIDGE_WAKE_TOPIC,UNIT_TO_PC), m_msg_buffer);
	return true;
}

// internal function to send data to the tuya controller
void tuya_bridge::send_cmd(uint8_t cmd, uint16_t len, uint8_t* d){
	char data[8+len];
	data[0]=0x55; // header
	data[1]=0xAA; // header
	data[2]=0x00; // Version
	data[3]=cmd;
	data[4]=len>>8;
	data[5]=len&0xff;
	if(len>0){
		for(uint16_t i=0;i<len;i++){
			data[6+i]=*d;
			d++;
		}
	}
	data[6+len]=0x00;
	for(uint16_t i=0;i<len+6;i++){
		data[6+len]+=data[i];
	}
	Serial.write(data,7+len);

	rfb_last_received=millis();
	//rfb.rdy=false;
	m_recv_state = RFB_START_0;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool tuya_bridge::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool tuya_bridge::publish(){
	// state before battery
	if(m_state.get_outdated()){
		logger.print(TOPIC_MQTT_PUBLISH, F("Tuya Reed  "), COLOR_GREEN);
		sprintf(m_msg_buffer,"%i",m_state.get_value());
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_TUYA_BRIDGE_REED_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_state.outdated(false);
		}
		return true; // yes, we've published something, stop others (avoid overfilling mqtt buffer)
	} else if(m_battery.get_outdated()){
		logger.print(TOPIC_MQTT_PUBLISH, F("Tuya Battery  "), COLOR_GREEN);
		sprintf(m_msg_buffer,"%i",m_battery.get_value());
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_TUYA_BRIDGE_BATTERY_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_battery.outdated(false);
		}
		return true; // yes, we've published something, stop others (avoid overfilling mqtt buffer)
	}
	return false;
}
#endif
