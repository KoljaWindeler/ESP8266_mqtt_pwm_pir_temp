// PRE
// RAM:   [======    ]  57.4% (used 47052 bytes from 81920 bytes)
// Flash: [=====     ]  48.7% (used 466556 bytes from 958448 bytes)

#include <cap_r503.h>
#ifdef WITH_HLW
r503::r503(){
	m_discovery_pub = false;
};

r503::~r503(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){

		undiscover(DOMAIN_BINARY_SENSOR,MQTT_R503_DINGDONG_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 DingDong config "), COLOR_YELLOW);
		 
		undiscover(DOMAIN_SENSOR,MQTT_R503_STATUS_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 Status config "), COLOR_YELLOW);
		
		undiscover(DOMAIN_SENSOR,MQTT_R503_CONFIDENCE_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 Confidence config "), COLOR_YELLOW);
		
		undiscover(DOMAIN_SENSOR,MQTT_R503_FINGER_ID_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 Finger config "), COLOR_YELLOW);
		
		undiscover(DOMAIN_SWITCH,MQTT_R503_TOUCHRING_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 Touchring config "), COLOR_YELLOW);
		
		undiscover(DOMAIN_SWITCH,MQTT_R503_SILENT_TOPIC);
		logger.println(TOPIC_MQTT_PUBLISH, F("Erasing R503 Silent config "), COLOR_YELLOW);
		
		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("r503 deleted"), COLOR_YELLOW);
};

uint8_t* r503::get_key(){
	return (uint8_t*)"R503";
}


bool r503::init(){
	uint8_t m_rxPin = 14; // D5
	uint8_t m_txPin = 13; // D2
	uint8_t m_bufferSize = 128;
	m_interruptPin = 12; //5; 
	m_DingDongPin = 5;
	m_relayPin = 4;
	m_fingerDetected = false;
	m_bellExecuted = false;
	m_blockUntilLift = false;
	m_touch_dingdong = false;
	m_silent_mode = false;
	m_lastsearch = millis();
	m_state = R503_STATE_IDLE;
	m_lastdindong = 0;
	pinMode(m_interruptPin,INPUT);
	pinMode(m_DingDongPin,INPUT_PULLUP);
	pinMode(m_relayPin,OUTPUT);
	swSer1 = new SoftwareSerial(m_rxPin, m_txPin, false, m_bufferSize);
	p_finger = new Adafruit_Fingerprint(swSer1);
	p_finger->begin(57600);
	logger.println(TOPIC_GENERIC_INFO, F("Testing for finger print reader"), COLOR_GREEN);
	if (p_finger->verifyPassword()) {
    	logger.println(TOPIC_GENERIC_INFO, F("Reader found"), COLOR_GREEN);
  	} else {
    	logger.println(TOPIC_GENERIC_INFO, F("Reader NOT found"), COLOR_RED);
	}
	return true;
}

uint8_t r503::count_intervall_update(){
	return 0; // 
}

void r503::error_to_logger(uint8 status){
	error_to_logger(status, false);
}

void r503::error_to_logger(uint8 status, bool verbose){
	if(status==FINGERPRINT_PACKETRECIEVEERR){
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint error packaget"), COLOR_RED);
	} else if(status==FINGERPRINT_IMAGEFAIL){
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint image fail"), COLOR_RED);
	} else if(status!=FINGERPRINT_NOFINGER){
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint unknown error"), COLOR_RED);
	} else if(status!=FINGERPRINT_IMAGEMESS){
		if(verbose){
			logger.println(TOPIC_GENERIC_INFO, F("Fingerprint too messy"), COLOR_RED);
		}
	} else if(status!=FINGERPRINT_FEATUREFAIL){
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint no feature found"), COLOR_RED);
	} else if(status!=FINGERPRINT_INVALIDIMAGE){
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint iamge invalid"), COLOR_RED);
	}
}

void r503::dingdong(){
	if(!m_bellExecuted){
		logger.println(TOPIC_GENERIC_INFO, F("Ding Dong"), COLOR_GREEN);
		p_finger->LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_PURPLE,3);
		if(millis()-m_lastdindong>2000){
			sprintf_P(m_msg_buffer, PSTR("ON"));
			network.publish(build_topic(MQTT_R503_DINGDONG_TOPIC,UNIT_TO_PC), m_msg_buffer);
			if(!m_silent_mode){
				digitalWrite(m_relayPin,HIGH);
				delay(100);
				digitalWrite(m_relayPin,LOW);
			}
			m_lastdindong = millis();
			sprintf_P(m_msg_buffer, PSTR("OFF"));
			network.publish(build_topic(MQTT_R503_DINGDONG_TOPIC,UNIT_TO_PC), m_msg_buffer);
		}
		m_bellExecuted = true;
	}
}

bool r503::loop(){
	// mDingDong goes low -> somebody pushed the button -> dingding
	// m_interruptPin goes low -> scanner detected something 
	//		see if the scanner can read a finger print ->
	//			if no: the finger is not correctly places -> act stpid -> dingdong
	//			if yes: blink 2x blue, check if the finger is known
	//				if no: blink 2x red, start over 


	if(millis()-m_lastsearch > 20){
	
		if(digitalRead(m_interruptPin)==LOW){
			if(!m_fingerDetected){
				m_fingerDetected = true;
				m_bellExecuted = false; // prepare for next
			}
			if(m_state == R503_STATE_ENROLE_WAIT_FOR_FINGER_1){
				int status = p_finger->getImage();
				if(status==FINGERPRINT_OK){
					p_finger->LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_BLUE,2);
					logger.println(TOPIC_GENERIC_INFO, F("Fingerprint 1 OK"), COLOR_GREEN);
					status = p_finger->image2Tz(1);
					if(status==FINGERPRINT_OK){
						sprintf_P(m_msg_buffer, PSTR("Finger %i OK, please lift"), 1);
						logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
						network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
						m_state = R503_STATE_ENROLE_WAIT_FOR_LIFT;
					} else {
						error_to_logger(status);
					} 
				} else {
					error_to_logger(status);
				} 
				//return true; // i did something
			} else if(m_state == R503_STATE_ENROLE_WAIT_FOR_LIFT) {
				int status = p_finger->getImage();
				if(status==FINGERPRINT_NOFINGER){
					sprintf_P(m_msg_buffer, PSTR("Finger lifted, please press again"));
					logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
					network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
					m_state = R503_STATE_ENROLE_WAIT_FOR_FINGER_2;
				} else {
					error_to_logger(status);
				}
				//return true; // i did something
			} else if(m_state == R503_STATE_ENROLE_WAIT_FOR_FINGER_2){
				int status = p_finger->getImage();
				if(status==FINGERPRINT_OK){
					p_finger->LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_BLUE,2);
					logger.println(TOPIC_GENERIC_INFO, F("Fingerprint 2 OK"), COLOR_GREEN);
					status = p_finger->image2Tz(2);
					if(status==FINGERPRINT_OK){
						logger.println(TOPIC_GENERIC_INFO, F("image to TZ 2 OK"), COLOR_GREEN);
						status = p_finger->createModel();
						if(status == FINGERPRINT_OK){
							logger.println(TOPIC_GENERIC_INFO, F("Create Model OK"), COLOR_GREEN);
							status = p_finger->storeModel(m_finger_id);
							if(status==FINGERPRINT_OK){
								// stored
								sprintf_P(m_msg_buffer, PSTR("Finger stored"));
								logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
								network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
								m_state = R503_STATE_IDLE;
							} else {
								error_to_logger(status);
							}
						} else if(status == FINGERPRINT_ENROLLMISMATCH){
							sprintf_P(m_msg_buffer, PSTR("Finger mismatch, try again"));
							logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_RED);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
							m_state = R503_STATE_ENROLE_WAIT_FOR_LIFT;
						} else {
							error_to_logger(status);
						}
					} else {
						error_to_logger(status);
					}
				} else {
					error_to_logger(status);
				}
				//return true; // i did something
			} else if(m_state == R503_STATE_IDLE && !m_blockUntilLift){
				int status = p_finger->getImage();
				if(status == FINGERPRINT_OK){
					logger.println(TOPIC_GENERIC_INFO, F("Fingerprint found"), COLOR_GREEN);
					p_finger->LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_BLUE,2);
					status = p_finger->image2Tz();
					if(status == FINGERPRINT_OK){
						logger.println(TOPIC_GENERIC_INFO, F("Image converted"), COLOR_GREEN);
						status = p_finger->fingerSearch();
						if(status == FINGERPRINT_OK){
							sprintf_P(m_msg_buffer, PSTR("Finger matched with id %i, confidence %i"),p_finger->fingerID,p_finger->confidence);
							p_finger->LEDcontrol(FINGERPRINT_LED_ON, 100, FINGERPRINT_LED_BLUE,2);
							logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
							sprintf_P(m_msg_buffer, PSTR("%i"),p_finger->confidence);
							network.publish(build_topic(MQTT_R503_CONFIDENCE_TOPIC,UNIT_TO_PC), m_msg_buffer);
							sprintf_P(m_msg_buffer, PSTR("%i"),p_finger->fingerID);
							network.publish(build_topic(MQTT_R503_FINGER_ID_TOPIC,UNIT_TO_PC), m_msg_buffer);
							// No ding ding?
							m_bellExecuted = true;
							m_blockUntilLift = true;
						} else if(status == FINGERPRINT_NOTFOUND){
							logger.println(TOPIC_GENERIC_INFO, F("Found NO match"), COLOR_RED);
							p_finger->LEDcontrol(FINGERPRINT_LED_FLASHING, 100, FINGERPRINT_LED_RED,2);
							if(m_touch_dingdong){
								// DING DONG!
								dingdong();
							}
						} else {
							error_to_logger(status);
						}
					} else {
						error_to_logger(status);
						// DING DONG!
					}
				} else {
					error_to_logger(status);
				}
			}
		} else if(m_fingerDetected && m_state == R503_STATE_IDLE){
			// finger is now gone
			if(m_blockUntilLift){
				m_blockUntilLift = false;
				p_finger->LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
			}
			if(!m_bellExecuted && m_touch_dingdong){
				dingdong();
			}
			m_fingerDetected = false;
			sprintf_P(m_msg_buffer, PSTR("Finger lifted"));
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
			sprintf_P(m_msg_buffer, PSTR("%i"),0);
			network.publish(build_topic(MQTT_R503_FINGER_ID_TOPIC,UNIT_TO_PC), m_msg_buffer);
		}
		
		// stupid dingdong input
		if(digitalRead(m_DingDongPin)==LOW){
			// no matter what: dingdong! even if we've already dingdong'ed and or we know the finger .. 
			m_bellExecuted = false;
			dingdong();
		} else {
			if(digitalRead(m_interruptPin)==HIGH){
				// button no longer pushed and no finger remaining on the scanner
				m_bellExecuted = false;
			}
		}

		m_lastsearch = millis();
	}
 	return false; // i did nothing
}

bool r503::intervall_update(uint8_t slot){
	return false;
}

bool r503::subscribe(){
	network.subscribe(build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	network.subscribe(build_topic(MQTT_R503_TOUCHRING_TOPIC,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_TOUCHRING_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	network.subscribe(build_topic(MQTT_R503_SILENT_TOPIC,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_SILENT_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	
	return true;
}

bool r503::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received R503 command"),COLOR_PURPLE);

		if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_RED, 7)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_BLUE, 8)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_PURPLE, 10)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_OFF, 7)){
			p_finger->LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_DELETE, 7)){
			if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_DELETE_ALL, 10)){
				p_finger->emptyDatabase();
				sprintf_P(m_msg_buffer, PSTR("Database cleared"));
				logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_RED);
				network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
			} else {
				for(uint8_t i=100;i>=0;i--){
					char* buffer = new char[strlen(MQTT_R503_COMMAND_DELETE)+4];
					sprintf(buffer,(const char *)MQTT_R503_COMMAND_DELETE,i);
					if (!strncmp((const char *) p_payload, buffer, strlen(buffer))){
						int status = p_finger->deleteModel(i);
						if(status == FINGERPRINT_OK){
							sprintf_P(m_msg_buffer, PSTR("Finger %i deleted"));
							logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
						} else {
							error_to_logger(status);
						}
					};
					delete[] buffer;
				}
			}
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_ENROLE, 7)){
			char* buffer = new char[strlen(MQTT_R503_COMMAND_ENROLE)+4];
			for(uint8_t i=100;i>=0;i--){
				sprintf(buffer,(const char *)MQTT_R503_COMMAND_ENROLE,i);
				if (!strncmp((const char *) p_payload, buffer, strlen(buffer))){
					m_state = R503_STATE_ENROLE_WAIT_FOR_FINGER_1;
					m_finger_id = i;
					sprintf_P(m_msg_buffer, PSTR("Start enrole, place finger"), 1);
					logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
					network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
					break;
				};
			}
			delete[] buffer;
		}

		if(strlen((const char *) p_payload)>0){
			network.publish(build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT), (char*)""); // avoid retrigger after reconnect
		}
		return true; /// msg processed
	} else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_TOUCHRING_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		sprintf_P(m_msg_buffer, PSTR("Touchring"));
		logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		if (!strncmp((const char *) p_payload, STATE_ON, 2)){
			m_touch_dingdong = true;
			sprintf_P(m_msg_buffer, PSTR("Touchring aktiviert"));
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_TOUCHRING_TOPIC, UNIT_TO_PC), (char *) STATE_ON);
		} else {
			m_touch_dingdong = false;
			sprintf_P(m_msg_buffer, PSTR("Touchring deaktiviert"));
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_TOUCHRING_TOPIC, UNIT_TO_PC), (char *) STATE_OFF);
		}
		return true; /// msg processed
	} else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_SILENT_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		if (!strncmp((const char *) p_payload, STATE_ON, 2)){
			m_silent_mode = true;
			network.publish(build_topic(MQTT_R503_SILENT_TOPIC, UNIT_TO_PC), (char *) STATE_ON);
			logger.println(TOPIC_GENERIC_INFO, F("Silent Mode activated"), COLOR_GREEN);
		} else {
			m_silent_mode = false;
			network.publish(build_topic(MQTT_R503_SILENT_TOPIC, UNIT_TO_PC), (char *) STATE_OFF);
			logger.println(TOPIC_GENERIC_INFO, F("Silent Mode deactivated"), COLOR_GREEN);
		}
	}
	
	return false; // not for me
}

bool r503::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool r503::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			if(discovery(DOMAIN_BINARY_SENSOR,MQTT_R503_DINGDONG_TOPIC,UNIT_NONE)){
				m_discovery_pub = true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 DingDong discovery"), COLOR_GREEN);
			}
			if(discovery(DOMAIN_SENSOR,MQTT_R503_STATUS_TOPIC,UNIT_NONE)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 Status discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			if(discovery(DOMAIN_SENSOR,MQTT_R503_CONFIDENCE_TOPIC,UNIT_NONE)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 Confidence discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			if(discovery(DOMAIN_SENSOR,MQTT_R503_FINGER_ID_TOPIC,UNIT_NONE)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 Finger discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			if(discovery(DOMAIN_SWITCH,MQTT_R503_SILENT_TOPIC,UNIT_NONE)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 Silent discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			if(discovery(DOMAIN_SWITCH,MQTT_R503_TOUCHRING_TOPIC,UNIT_NONE)){
				m_discovery_pub &= true;
				logger.println(TOPIC_MQTT_PUBLISH, F("R503 Touchring discovery"), COLOR_GREEN);
			} else {
				m_discovery_pub &= false;
			}
			return true;
		}
	}
#endif
 	return false;
}
#endif
