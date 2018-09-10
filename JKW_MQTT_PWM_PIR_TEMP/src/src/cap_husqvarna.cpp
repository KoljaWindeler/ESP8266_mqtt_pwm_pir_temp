#include <cap_husqvarna.h>

// simply the constructor
husqvarna::husqvarna(){
	sprintf((char*)key,"HUS");
	minute_counter=0;
};

// simply the destructor
husqvarna::~husqvarna(){
	logger.println(TOPIC_GENERIC_INFO, F("husqvarna deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded undetr all circumstances
bool husqvarna::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* husqvarna::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool husqvarna::init(){
	logger.println(TOPIC_GENERIC_INFO, F("husqvarna init"), COLOR_GREEN);
	swSer1 = new SoftwareSerial(14, 12, false, 25);
	swSer1->begin(9600);
	return true;
}

// return how many value you want to publish per second
// e.g. DHT22: Humidity + Temp = 2
uint8_t husqvarna::count_intervall_update(){
	return 1; // technically we're not publishing anything but we need to be called on a fixed time base
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool husqvarna::loop(){
	// a) enough data b) we lost some, so we timeout
	if(swSer1->available()>=5 || (millis()-last_received>5000 && swSer1->available()>0)){
		uint8_t char_num=0;
		uint8_t char_in;
		memcpy(response,0x00,5); // erase
		while(swSer1->available()){
			char_in=swSer1->read();

			// DBG
			sprintf(m_msg_buffer,"received husqvarna cmd[%i]=%02x",char_num,char_in);
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_YELLOW);

			response[char_num]=char_in;
			char_num=min(char_num+1,4); // limit it [0..4]
		}
		// only set the state if we received enough data
		if(char_num==4){
			// DBG
			sprintf(m_msg_buffer,"received husqvarna cmd %02x %02x %02x %02x %02x",response[0],response[1],response[2],response[3],response[4]);
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_YELLOW);

			// (auto, manual, home) only first 3 byte, yes (all 5 byte)
			if(m_last_cmd.get_value()==CMD_MODE || m_last_cmd.get_value()==CMD_YES_KEY){
				if(response[0]==last_command_send[0] && response[2]==last_command_send[2] &&
					response[3]==last_command_send[4] && response[4]==last_command_send[3]){ // byte 3 and 4 are swapped
						m_state.set(1); // set output and outdated
					} else {
						m_state.set(0); // set output and outdated
					}
			}
			else {
				m_state.set(((uint16_t)response[4]) << 8 | response[3]);
			}
		}
		//sprintf(temp,"in: %02x state %i",t,m_state);
		//logger.pln(temp);
	}
	return false; // i did nothing that shouldn't be non-interrupted
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool husqvarna::intervall_update(uint8_t slot){
	// request status every 5 minute
	minute_counter=(minute_counter+1)%5;
	if(minute_counter==0){
		m_last_cmd.set(CMD_REQUEST_STATUS);
		last_command_send = (uint8_t*)request_status_cmd;
		send_cmd(last_command_send);
	} else if(minute_counter==1){
		m_last_cmd.set(CMD_CAPACITY);
		last_command_send = (uint8_t*)battery_capacity_cmd;
		send_cmd(last_command_send);
	} else if(minute_counter==2){
		m_last_cmd.set(CMD_VOLTAGE);
		last_command_send = (uint8_t*)battery_voltage_cmd;
		send_cmd(last_command_send);
	} else if(minute_counter==3){
		m_last_cmd.set(CMD_TEMP);
		last_command_send = (uint8_t*)battery_temp_cmd;
		send_cmd(last_command_send);
	} else if(minute_counter==4){
		m_last_cmd.set(CMD_TIME_SINCE_CHARGE);
		last_command_send = (uint8_t*)time_since_charging_cmd;
		send_cmd(last_command_send);
	}
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool husqvarna::subscribe(){
	network.subscribe(build_topic(MQTT_HUSQVARNA_MODE_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_HUSQVARNA_MODE_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

void husqvarna::send_cmd(uint8_t* cmd){
	// DBG
	sprintf(m_msg_buffer,"sending husqvarna cmd %02x %02x %02x %02x %02x",cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_YELLOW);

	swSer1->write(cmd[0]);
	swSer1->write(cmd[1]);
	swSer1->write(cmd[2]);
	swSer1->write(cmd[3]);
	swSer1->write(cmd[4]);
	last_received=millis();
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool husqvarna::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_HUSQVARNA_MODE_TOPIC,PC_TO_UNIT))) {

		logger.print(TOPIC_GENERIC_INFO, F("husqvarna command received: "), COLOR_GREEN);
		logger.pln((char*)p_payload);

		if (!strcmp_P((const char *) p_payload, PSTR("AUTO"))) {
			m_last_cmd.set(CMD_MODE);
			last_command_send = (uint8_t*)auto_mode_cmd;
		} else if (!strcmp_P((const char *) p_payload, PSTR("HOME"))) {
			m_last_cmd.set(CMD_MODE);
			last_command_send = (uint8_t*)goHome_cmd;
		} else if (!strcmp_P((const char *) p_payload, PSTR("MANUAL"))) {
			m_last_cmd.set(CMD_MODE);
			last_command_send = (uint8_t*)manuell_mode_cmd;
		}
		send_cmd(last_command_send);
		return true;
	}
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool husqvarna::publish(){
	if(m_state.get_outdated()){
		logger.print(TOPIC_MQTT_PUBLISH, F("Publishing "), COLOR_GREEN);

		// STATUS
		if(m_last_cmd.get_value()==CMD_REQUEST_STATUS){
			if(m_state.get_value()==6){
				sprintf_P(m_msg_buffer,PSTR("Left motor blocked"));
			} else if(m_state.get_value()==12){
				sprintf_P(m_msg_buffer,PSTR("No border signal"));
			} else if(m_state.get_value()==18){
				sprintf_P(m_msg_buffer,PSTR("Low voltage"));
			} else if(m_state.get_value()==26){
				sprintf_P(m_msg_buffer,PSTR("Chargingstation blocked"));
			} else if(m_state.get_value()==34){
				sprintf_P(m_msg_buffer,PSTR("Mower lifted"));
			} else if(m_state.get_value()==52){
				sprintf_P(m_msg_buffer,PSTR("Chargingstation disconnected"));
			} else if(m_state.get_value()==54){
				sprintf_P(m_msg_buffer,PSTR("PIN elapsed"));
			} else if(m_state.get_value()==1000){
				sprintf_P(m_msg_buffer,PSTR("Leaving chargingstation"));
			} else if(m_state.get_value()==1002){
				sprintf_P(m_msg_buffer,PSTR("Mowing"));
			} else if(m_state.get_value()==1006){
				sprintf_P(m_msg_buffer,PSTR("Mower starting"));
			} else if(m_state.get_value()==1012){
				sprintf_P(m_msg_buffer,PSTR("Mower started"));
			} else if(m_state.get_value()==1014){
				sprintf_P(m_msg_buffer,PSTR("Charging"));
			} else if(m_state.get_value()==1016){
				sprintf_P(m_msg_buffer,PSTR("Waiting in charger"));
			} else if(m_state.get_value()==1024){
				sprintf_P(m_msg_buffer,PSTR("Returning to charger"));
			} else if(m_state.get_value()==1036){
				sprintf_P(m_msg_buffer,PSTR("Square mode"));
			} else if(m_state.get_value()==1038 || m_state.get_value()==1062){
				sprintf_P(m_msg_buffer,PSTR("Stuck"));
			} else if(m_state.get_value()==1040){
				sprintf_P(m_msg_buffer,PSTR("Collision"));
			} else if(m_state.get_value()==1042 || m_state.get_value()==1064){
				sprintf_P(m_msg_buffer,PSTR("Searching"));
			} else if(m_state.get_value()==1044){
				sprintf_P(m_msg_buffer,PSTR("Stop"));
			} else if(m_state.get_value()==1048){
				sprintf_P(m_msg_buffer,PSTR("Docking"));
			} else if(m_state.get_value()==1052){
				sprintf_P(m_msg_buffer,PSTR("Error"));
			} else if(m_state.get_value()==1056){
				sprintf_P(m_msg_buffer,PSTR("Waiting on action"));
			} else if(m_state.get_value()==1058 || m_state.get_value()==1072) {
				sprintf_P(m_msg_buffer,PSTR("Following border"));
			} else if(m_state.get_value()==1060){
				sprintf_P(m_msg_buffer,PSTR("N-Signal found"));
			} else if(m_state.get_value()==1070){
				sprintf_P(m_msg_buffer,PSTR("Following search line"));
			} else {
				sprintf_P(m_msg_buffer,PSTR("unknown state"));
			}
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}

		// mode or KEY
		else if(m_last_cmd.get_value()==CMD_YES_KEY || m_last_cmd.get_value()==CMD_MODE){
			if(m_state.get_value()==1){
				sprintf_P(m_msg_buffer,PSTR("command ok"));
			} else {
				sprintf_P(m_msg_buffer,PSTR("command failed"));
			}
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}

		// capacity
		else if(m_last_cmd.get_value()==CMD_CAPACITY){
			sprintf_P(m_msg_buffer,PSTR("%lu mAh"),m_state.get_value());
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_BATTERY_CAP_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}

		// voltage
		else if(m_last_cmd.get_value()==CMD_VOLTAGE){
			sprintf_P(m_msg_buffer,PSTR("%lu mV"),m_state.get_value());
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_BATTERY_VOLTAGE_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}

		// temp
		else if(m_last_cmd.get_value()==CMD_TEMP){
			sprintf_P(m_msg_buffer,PSTR("%lu deg"),m_state.get_value());
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_BATTERY_TEMP_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}

		// time
		else if(m_last_cmd.get_value()==CMD_TIME_SINCE_CHARGE){
			sprintf_P(m_msg_buffer,PSTR("%lu sec"),m_state.get_value());
			// publishing
			logger.pln(m_msg_buffer);
			if(network.publish(build_topic(MQTT_HUSQVARNA_BATTERY_TIME_TOPIC,UNIT_TO_PC), m_msg_buffer)){
				m_state.outdated(false);
			}
		}
		// at least tried
		return true;
	}
	return false;
}
