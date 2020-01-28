#include <cap_shelly_dimmer.h>
#ifdef WITH_SHELLY_DIMMER
// simply the constructor
shelly_dimmer::shelly_dimmer(){};

// simply the destructor
shelly_dimmer::~shelly_dimmer(){
	logger.println(TOPIC_GENERIC_INFO, F("shelly_dimmer deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool shelly_dimmer::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}

// the will be requested to check if the key is in the config strim
uint8_t* shelly_dimmer::get_key(){
	return (uint8_t*)"SHD";
}

// will be callen if the key is part of the config
bool shelly_dimmer::init(){
	logger.enable_mqtt_trace(true);
	logger.println(TOPIC_GENERIC_INFO, F("shelly_dimmer init"), COLOR_GREEN);
	logger.enable_serial_trace(false);
	Serial.end();
	delay(50);
	Serial.begin(115200);

	// ready to listen
	m_recv_state = SHD_START;
	m_last_char_recv = 0;

	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t shelly_dimmer::count_intervall_update(){
	return 0; // handled in loop and pushed directly
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool shelly_dimmer::loop(){
	// state machine to parse incoming data
	if(Serial.available()){
		t=Serial.read();
		if(millis()-m_last_char_recv>600){ // typically we have 1989 ms between last char of a status request and first .. 
		   m_recv_state = SHD_START;
		}
		m_last_char_recv = millis();
		//Serial.printf("Received %02x, state %i\r\n",t,m_recv_state);
		if(m_recv_state == SHD_START){
			if(t == 0x01){ // start byte
				//logger.pln("s");
				m_msg_in.start = t;
				m_recv_state++;
				m_msg_in.chk = t;
			}
		} else if(m_recv_state == SHD_ID){
			m_msg_in.id = t;
			m_recv_state++;
			m_msg_in.chk += t;
		} else if(m_recv_state == SHD_CMD){
			//logger.pln("v");
			m_msg_in.cmd = t;
			m_recv_state++;
			m_msg_in.chk += t;
		} else if(m_recv_state == SHD_LEN){
			//logger.pln("cmd");
			m_msg_in.len = t;
			m_msg_in.data_p = 0;
			m_recv_state++; // data .. 
			if(m_msg_in.len == 0){
				m_recv_state++; // if no data .. goto chk
			}
			m_msg_in.chk += t;
		} else if(m_recv_state == SHD_DATA){
			//logger.pln("len");
			m_msg_in.data[m_msg_in.data_p] = t;
			m_msg_in.data_p++;
			//Serial.printf("recv %i / %i \r\n",m_msg_in.data_p,m_msg_in.len);
			m_msg_in.chk += t;
			if(m_msg_in.data_p>sizeof(m_msg_in.data)-1){
				m_msg_in.data_p = 0;
			}
			if(m_msg_in.data_p==m_msg_in.len){
				m_recv_state++;
			}
		} else if(m_recv_state == SHD_CHK_1){
			//logger.pln("chk1");
			m_msg_in.chk -= 1; 
			if((m_msg_in.chk>>8)==t){
				//logger.pln("ok");
				m_recv_state++;
			} else {
				m_recv_state = SHD_START;
			}
		} else if(m_recv_state == SHD_CHK_2){
			//logger.pln("chk2");
			if((m_msg_in.chk & 0xff)==t){
				//logger.pln("ok, message ok");
				if(m_msg_in.cmd==0x10){
					uint32_t power_raw = (((uint32_t)m_msg_in.data[7])<<24)+(((uint32_t)m_msg_in.data[6])<<16)+(((uint16_t)m_msg_in.data[5])<<8)+m_msg_in.data[4];			
					if(power_raw){
						m_power.set((uint16_t)(1000000UL/power_raw));
					} else {
						m_power.set(0);
					}
				} else if(m_msg_in.cmd != 0x01) { // no need to see all dimming messages, they will just flood the console when we dimm ourself
					sprintf(m_msg_buffer,"Recv CMD: 0x%02x Length: 0x%02x -> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",m_msg_in.cmd, m_msg_in.len, m_msg_in.data[0],m_msg_in.data[1],m_msg_in.data[2],m_msg_in.data[3],m_msg_in.data[4],m_msg_in.data[5],m_msg_in.data[6],m_msg_in.data[7],m_msg_in.data[8],m_msg_in.data[9],m_msg_in.data[10],m_msg_in.data[11]);
					logger.p(m_msg_buffer);
				}
			}
			m_recv_state = SHD_START;
		}
		//Serial.printf("Next state %i\r\n",m_recv_state);
	} else if(millis()-m_last_char_recv>SHELLY_DIMMER_POWER_UPDATE_RATE){ // 10 sec update 
		send_cmd(0x10,0,(uint8_t*)m_msg_buffer);
		m_last_char_recv = millis(); // avoid resend
	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool shelly_dimmer::intervall_update(uint8_t slot){
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool shelly_dimmer::subscribe(){
	return false;
}

uint8_t* shelly_dimmer::get_dep(){
	return (uint8_t*)"LIG";
}

uint8_t shelly_dimmer::get_modes(){
	return (1<<SUPPORTS_PWM); 
};

void shelly_dimmer::print_name(){
	logger.pln(F("Shelly dimmer"));
};


void shelly_dimmer::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t px){
	// input will be 0..255, so we have to wrap that to 0..1000
	uint16_t r_scale = min(r*4,1000);
	uint8_t data[2];
	data[0]=r_scale & 0xff;
	data[1]=r_scale >> 8;
	send_cmd(1,2,data);
}

void shelly_dimmer::send_cmd(uint8_t cmd,uint8_t len,uint8_t *payload){
	send_cmd(cmd,len,payload,false);
}

void shelly_dimmer::send_cmd(uint8_t cmd,uint8_t len,uint8_t *payload, boolean debug){
	char data[len+7];
	uint16_t chk = -1;
	data[0]=0x01;				// fix
	data[1]=m_msg_in.id+1;		// id
	data[2]=cmd;				// cmd e.g. 0x01= set brightness
	data[3]=len;				// len
	for(uint8_t i=0;i<len;i++){
		data[4+i]=payload[i];
	}
	// calc chk
	for(uint8_t i=0;i<4+len;i++){
		chk+=data[i];
	}
	data[len+4]=chk >> 8;		// high nibble  
	data[len+5]=chk & 0xff;		// low nibble
	data[len+6]=0x04;			// fix
	Serial.write(data,len+7);

	if(debug){
		sprintf(m_msg_buffer,"Sending CMD: 0x%02x Length: 0x%02x -> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n",m_msg_in.cmd, m_msg_in.len, m_msg_in.data[0],m_msg_in.data[1],m_msg_in.data[2],m_msg_in.data[3],m_msg_in.data[4],m_msg_in.data[5],m_msg_in.data[6],m_msg_in.data[7],m_msg_in.data[8],m_msg_in.data[9],m_msg_in.data[10],m_msg_in.data[11]);
		logger.p(m_msg_buffer);
	}
}


// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool shelly_dimmer::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool shelly_dimmer::publish(){
	if(m_power.get_outdated()){
		logger.print(TOPIC_MQTT_PUBLISH, F("Shelly Power "), COLOR_GREEN);
		sprintf(m_msg_buffer,"%i",m_power.get_value());
		logger.pln(m_msg_buffer);
		if(network.publish(build_topic(MQTT_SHELLY_DIMMER_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer)){
			m_power.outdated(false);
		}
		return true;
	}
	return false;
}
#endif
