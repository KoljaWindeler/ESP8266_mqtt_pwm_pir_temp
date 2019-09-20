#include <cap_bridge.h>

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
	logger.enable_mqtt_trace(true);
	logger.println(TOPIC_GENERIC_INFO, F("Bridge init"), COLOR_GREEN);
	logger.enable_serial_trace(false);
	Serial.end();
	delay(1000);
	Serial.begin(9600);

	m_msg_state = 0;
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
	if(m_msg_state%2==0){ // ich bin
		if(m_msg_state==0){
			send_cmd(1, 0, (uint8_t*)"");// request generic info
			logger.println(TOPIC_GENERIC_INFO,"generic request send",COLOR_GREEN);
			//m_msg_state=1; // wait for reply
			m_msg_state=7; // wait for reply
		}

		/*else if(m_msg_state==2){
			uint8_t test3[8]={0x55,0xAA,0x00,0x02,0x00,0x01,0x02,0x04};
			Serial.write(test3,8);
			rfb_last_received=millis();
			m_msg_state++; // wait for reply
		}

		else if(m_msg_state==4){
			uint8_t test4[8]={0x55,0xAA,0x00,0x02,0x00,0x01,0x02,0x04};
			Serial.write(test4,8);
			rfb_last_received=millis();
			m_msg_state++; // wait for reply
		}

		else if(m_msg_state==6){
			uint8_t test5[8]={0x55,0xAA,0x00,0x02,0x00,0x01,0x03,0x05};
			Serial.write(test5,8);
			rfb_last_received=millis();
			m_msg_state++; // wait for reply
		}*/

		else if(m_msg_state==8){
			uint8_t a=0x04;
			send_cmd(2, 1, &a);
			logger.println(TOPIC_GENERIC_INFO,"status x04 send",COLOR_GREEN);
			m_msg_state++; // wait for status
		}
	}

	if(Serial.available() && !rfb.rdy){
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
			//logger.pln("chk");
			if(rfb.chk==t){
				//logger.pln("ok");
				//char m[80];
				//sprintf(m,"len: %i, cmd: %i, [chk]:%x",rfb.len,rfb.cmd,rfb.chk);
				if(rfb.cmd==5 && rfb.len>=18){
					status.batt = rfb.data[17];
					status.tamper = rfb.data[9];
					status.reed = rfb.data[4];
				}
				//logger.pln(m);
				rfb.rdy=true;
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

void bridge::send_cmd(uint8_t cmd, uint16_t len, uint8_t* d){
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
	rfb.rdy=false;
	m_recv_state = RFB_START_0;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool bridge::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool bridge::publish(){
	if(rfb.rdy){
		logger.print(TOPIC_MQTT_PUBLISH, F("incoming rf data "), COLOR_GREEN);
		sprintf(m_msg_buffer,"%02x%02x%02x",rfb.data[17],rfb.data[9],rfb.data[4]); // bat, tamper, switch
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_BRIDGE_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			rfb.rdy=false;
		}
		return true; // yes, we've published something, stop others (avoid overfilling mqtt buffer)
	}
	return false;
}
