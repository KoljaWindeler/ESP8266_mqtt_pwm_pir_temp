// PRE
// RAM:   [======    ]  57.4% (used 47052 bytes from 81920 bytes)
// Flash: [=====     ]  48.7% (used 466556 bytes from 958448 bytes)

// POST
// RAM:   [======    ]  57.8% (used 47360 bytes from 81920 bytes)
// Flash: [=====     ]  49.4% (used 473396 bytes from 958448 bytes)

// PROGMEM 6840

#include <cap_r503.h>
#ifdef WITH_HLW
r503::r503(){
	m_discovery_pub = false;
	m_raw_template = nullptr;
	m_download_packet_id = 0;
};

r503::~r503(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		// auto discovery
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
	uint8_t m_bufferSize = 140; // fingerprint download alone has 128+9 byte length
	m_interruptPin = 12; //5; 
	m_DingDongPin = 5;
	m_relayPin = 4;
	m_preenrole_id = 0;
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
		logger.println(TOPIC_GENERIC_INFO, F("Fingerprint image invalid"), COLOR_RED);
	} else if(status!=FINGERPRINT_DBRANGEFAIL){
		logger.println(TOPIC_GENERIC_INFO, F("ID out of range"), COLOR_RED);
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
						logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
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
					logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
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
								logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
								network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
								m_state = R503_STATE_IDLE;
							} else {
								error_to_logger(status);
							}
						} else if(status == FINGERPRINT_ENROLLMISMATCH){
							sprintf_P(m_msg_buffer, PSTR("Finger mismatch, try again"));
							logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_RED);
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
							sprintf_P(m_msg_buffer, PSTR("Finger matched with id %i, confidence %i"),p_finger->fingerID,int(p_finger->confidence/2.55));
							p_finger->LEDcontrol(FINGERPRINT_LED_ON, 100, FINGERPRINT_LED_BLUE,2);
							logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
							sprintf_P(m_msg_buffer, PSTR("%i"),int(p_finger->confidence/2.55));
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
	// reminder: only add extra topics here, all switches / input button that are sent via discovery will auto-subscribe
	network.subscribe(build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT), COLOR_GREEN);

	network.subscribe(build_topic(MQTT_R503_COMMAND_UPLOAD_FINGER,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_COMMAND_UPLOAD_FINGER,PC_TO_UNIT), COLOR_GREEN);

	network.subscribe(build_topic(MQTT_R503_COMMAND_DOWNLOAD_FINGER,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_R503_COMMAND_DOWNLOAD_FINGER,PC_TO_UNIT), COLOR_GREEN);
	
	
	return true;
}

bool r503::receive(uint8_t* p_topic, uint8_t* p_payload){
	// command topic, multi input possible
	if (!strcmp((const char *) p_topic, build_topic(MQTT_R503_COMMAND_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received R503 command"),COLOR_PURPLE);

		// LED COLOR setting
		if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_RED, 7)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_RED);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_BLUE, 8)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_PURPLE, 10)){
			p_finger->LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_PURPLE);
		} else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_LED_OFF, 7)){
			p_finger->LEDcontrol(FINGERPRINT_LED_OFF, 0, FINGERPRINT_LED_RED);
		} 
		
		// Delte one or all finger from storage
		else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_DELETE, 7)){
			if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_DELETE_ALL, 10)){
				p_finger->emptyDatabase();
				sprintf_P(m_msg_buffer, PSTR("Database cleared"));
				logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_RED);
				network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
			} else {
				for(int8_t i=100;i>=0;i--){
					char* buffer = new char[strlen(MQTT_R503_COMMAND_DELETE)+4];
					sprintf(buffer,(const char *)MQTT_R503_COMMAND_DELETE,i);
					if (!strncmp((const char *) p_payload, buffer, strlen(buffer))){
						int status = p_finger->deleteModel(i);
						if(status == FINGERPRINT_OK){
							sprintf_P(m_msg_buffer, PSTR("Finger %i deleted"),i);
							logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
						} else {
							error_to_logger(status);
						}
					};
					delete[] buffer;
				}
			}
		} 
		
		// Enrole a fingerprint, directly to a slot
		else if (!strncmp((const char *) p_payload, MQTT_R503_COMMAND_ENROLE, 7)){
			char* buffer = new char[strlen(MQTT_R503_COMMAND_ENROLE)+4];
			for(int8_t i=100;i>=0;i--){
				sprintf(buffer,(const char *)MQTT_R503_COMMAND_ENROLE,i);
				if (!strncmp((const char *) p_payload, buffer, strlen(buffer))){
					m_state = R503_STATE_ENROLE_WAIT_FOR_FINGER_1;
					m_finger_id = i;
					sprintf_P(m_msg_buffer, PSTR("Start enrole, place finger"), 1);
					logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
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
	} 
	
	// switch for Touchring Topic, ring on touch of a invalid finger
	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_TOUCHRING_TOPIC,PC_TO_UNIT))) { 
		sprintf_P(m_msg_buffer, PSTR("Touchring"));
		logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		if (!strncmp((const char *) p_payload, STATE_ON, 2)){
			m_touch_dingdong = true;
			sprintf_P(m_msg_buffer, PSTR("Touchring aktiviert"));
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_TOUCHRING_TOPIC, UNIT_TO_PC), (char *) STATE_ON);
		} else {
			m_touch_dingdong = false;
			sprintf_P(m_msg_buffer, PSTR("Touchring deaktiviert"));
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_TOUCHRING_TOPIC, UNIT_TO_PC), (char *) STATE_OFF);
		}
		return true; /// msg processed
	} 
	
	// switch for silent, don't ring on push
	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_SILENT_TOPIC,PC_TO_UNIT))) {
		if (!strncmp((const char *) p_payload, STATE_ON, 2)){
			m_silent_mode = true;
			network.publish(build_topic(MQTT_R503_SILENT_TOPIC, UNIT_TO_PC), (char *) STATE_ON);
			logger.println(TOPIC_MQTT_PUBLISH, F("Silent Mode activated"), COLOR_GREEN);
		} else {
			m_silent_mode = false;
			network.publish(build_topic(MQTT_R503_SILENT_TOPIC, UNIT_TO_PC), (char *) STATE_OFF);
			logger.println(TOPIC_MQTT_PUBLISH, F("Silent Mode deactivated"), COLOR_GREEN);
		}
		return true; /// msg processed
	} 
	
	// input to prepare the enrole process 
	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_PREENROLE_COMMAND_TOPIC,PC_TO_UNIT))) {
		m_preenrole_id = atoi((char*) p_payload);
		if(m_preenrole_id<0 || m_preenrole_id>100){
			m_preenrole_id = 0;
		}
		sprintf(m_msg_buffer,"%i",m_preenrole_id);
		network.publish(build_topic(MQTT_R503_PREENROLE_COMMAND_TOPIC, UNIT_TO_PC), (char *) m_msg_buffer);
		logger.println(TOPIC_MQTT_PUBLISH, F("Return preenrole ID"), COLOR_GREEN);

		// check if slot is empty .. this is not a prerequisit, just helpfull info
		uint8_t status = p_finger->loadModel(m_preenrole_id);
		if(status == FINGERPRINT_OK){
			sprintf_P(m_msg_buffer, PSTR("ID in use"), 1);
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		} else if(status == FINGERPRINT_DBRANGEFAIL){
			sprintf_P(m_msg_buffer, PSTR("ID free"), 1);
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		} else {
			error_to_logger(status);
		};
		return true; /// msg processed
	} 
	
	// execute the enrolement (or stop it again)
	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_ENROLE_COMMAND_TOPIC,PC_TO_UNIT))) { // considering to make this a switch, not a button
		logger.println(TOPIC_GENERIC_INFO, F("enrole command received"), COLOR_GREEN);
		if(m_preenrole_id>0 && m_state==R503_STATE_IDLE){
			m_state = R503_STATE_ENROLE_WAIT_FOR_FINGER_1;
			m_finger_id = m_preenrole_id;
			sprintf_P(m_msg_buffer, PSTR("Start enrole, place finger"), 1);
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		} else if(m_state>R503_STATE_IDLE){
			m_state = R503_STATE_IDLE;
			sprintf_P(m_msg_buffer, PSTR("Enrolement canceled"), 1);
			logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);
			network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		}
		return true; /// msg processed
	}
	
	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_COMMAND_UPLOAD_FINGER,PC_TO_UNIT))) { // considering to make this a switch, not a button
		logger.println(TOPIC_GENERIC_INFO, F("dump command received"), COLOR_GREEN);
		m_preenrole_id = atoi((char*) p_payload);
		if(m_preenrole_id<0 || m_preenrole_id>100){
			m_preenrole_id = 0;
		}
		uint8_t status = p_finger->loadModel(m_preenrole_id);
		if(status == FINGERPRINT_OK){
			sprintf_P(m_msg_buffer, PSTR("Model loaded"), 1);
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
			
			// Jetzt den Sendevorgang am Sensor anstoßen
			uint8_t repeat_dump = 2;
			while(repeat_dump>0){
				repeat_dump--; 
				status = p_finger->getModel();
				if(status == FINGERPRINT_OK){
					if(m_raw_template == nullptr){
						m_raw_template = (uint8_t*)malloc(TEMPLATE_SIZE);
					}
					if (m_raw_template != nullptr) {
						uint16_t bytes_copied = 0;
						bool dump_success = false;			
						Adafruit_Fingerprint_Packet reply_packet(0, 0, NULL); 
		
		
						while (bytes_copied < TEMPLATE_SIZE) {
							status = p_finger->getStructuredPacket(&reply_packet, 500); // get each packet with up to 500ms
							
							if (status == FINGERPRINT_OK) {
								if (reply_packet.type == FINGERPRINT_DATAPACKET || reply_packet.type == FINGERPRINT_ENDDATAPACKET) {
									// copy to storage, direct transfer via WIFI wasn't fast enough 
									for (int i = 0; i < reply_packet.length-2; i++) { // 2 byte checksum is part of "length"
										if (bytes_copied < TEMPLATE_SIZE) {
											m_raw_template[bytes_copied] = reply_packet.data[i];
											bytes_copied++;
										}
									}
										
									sprintf_P(m_msg_buffer, PSTR("Dump OK. Total: %d Bytes"), bytes_copied);
									logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
									
									// War das das letzte Paket?
									if (reply_packet.type == FINGERPRINT_ENDDATAPACKET) {
										dump_success = true;
										sprintf_P(m_msg_buffer, PSTR("Transfer completed"));
										logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
										break;
									}
								} else {
									// in case we get stuff like ACK which we shouldn't
									status = FINGERPRINT_BADPACKET;
									break;
								}
							} else {
								// dump failed
								error_to_logger(status);
								break;
							}
						}
		
						if(dump_success && bytes_copied == TEMPLATE_SIZE){
							sprintf_P(m_msg_buffer, PSTR("Model dumped. Size: %d Bytes"), bytes_copied);
							logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
	
							/*Serial.println(F("--- RAM TEMPLATE INSPECTION ---"));
							for (uint16_t x = 0; x < TEMPLATE_SIZE; x++) {
								if (m_raw_template[x] < 0x10) Serial.print('0');
								Serial.print(m_raw_template[x], HEX);
								Serial.print(' ');
								if ((x + 1) % 32 == 0) Serial.println(); // Zeilenumbruch alle 32 Bytes
							}
							Serial.println(F("-------------------------------"));*/
							
							for(uint16_t i=0;i<bytes_copied;i+=TEMPLATE_CHUNK_SIZE){
								sprintf(m_msg_buffer,"%02X",i/TEMPLATE_CHUNK_SIZE);
								if(i+TEMPLATE_CHUNK_SIZE>TEMPLATE_SIZE){
									encodeBase64(m_raw_template+i,TEMPLATE_SIZE-i,m_msg_buffer+2);
								} else {
									encodeBase64(m_raw_template+i,TEMPLATE_CHUNK_SIZE,m_msg_buffer+2);
								}
								logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
								network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
								yield();
								delay(10);
							}
							repeat_dump = 0; // no further retires needed
							
						} else {
							sprintf_P(m_msg_buffer, PSTR("Model loaded ok but dump failed"), 1);
							logger.println(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_RED);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
							if(repeat_dump>0){
								sprintf_P(m_msg_buffer, PSTR("trying again in 3s"));
								logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_PURPLE);
								network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
								delay(3000);
							} else {
								sprintf_P(m_msg_buffer, PSTR("giving up"));
								logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_RED);
								network.publish(build_topic(MQTT_R503_STATUS_TOPIC,UNIT_TO_PC), m_msg_buffer);
							}
						}
						free(m_raw_template);
						m_raw_template = nullptr;
					} else {
						logger.println(TOPIC_GENERIC_INFO, F("Kein RAM für Template!"), COLOR_RED);
					}
				} else {
					error_to_logger(status);
				};
			}
		} else {
			error_to_logger(status);
		};
		return true;
	}

	else if(!strcmp((const char *) p_topic, build_topic(MQTT_R503_COMMAND_DOWNLOAD_FINGER,PC_TO_UNIT))) {
		//logger.println(TOPIC_GENERIC_INFO, F("download (pc to unit) command received"), COLOR_GREEN);

		uint8_t packet_id = (hexCharToVal(p_payload[0])<<4) | hexCharToVal(p_payload[1]); 
		if(packet_id == m_download_packet_id){
			//logger.println(TOPIC_GENERIC_INFO, F("Packet ID ok"), COLOR_GREEN);
			if(m_raw_template == nullptr){
				//logger.println(TOPIC_GENERIC_INFO, F("create buffer"), COLOR_GREEN);
				m_raw_template = (uint8_t*)malloc(TEMPLATE_SIZE);
			}
			if (m_raw_template != nullptr) {
				//logger.println(TOPIC_GENERIC_INFO, F("buffer ok"), COLOR_GREEN);
				decodeBase64((const char*)(p_payload + 2), m_raw_template + packet_id*30);
				
				sprintf_P(m_msg_buffer, PSTR("Chunk %d/51 saved to RAM"), packet_id);
        		logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);

				// 4. War das der letzte Chunk?
				if (packet_id == TEMPLATE_SIZE/TEMPLATE_CHUNK_SIZE) {
					/*
					Serial.println(F("--- RAM TEMPLATE INSPECTION ---"));
					for (uint16_t x = 0; x < TEMPLATE_SIZE; x++) {
						if (m_raw_template[x] < 0x10) Serial.print('0');
						Serial.print(m_raw_template[x], HEX);
						Serial.print(' ');
						if ((x + 1) % 32 == 0) Serial.println(); // Zeilenumbruch alle 32 Bytes
					}
					Serial.println(F("-------------------------------"));
					*/
					logger.println(TOPIC_GENERIC_INFO, F("All chunks received! Writing to sensor..."), COLOR_GREEN);
					
					// Schritt A: Template in den Charakter-Buffer 1 des R503 schreiben
					uint8_t status = p_finger->putModel(1, m_raw_template, TEMPLATE_SIZE);
					
					if (status == FINGERPRINT_OK) {
						logger.println(TOPIC_GENERIC_INFO, F("Template successfully written to sensor buffer 1"), COLOR_GREEN);
						
						// Schritt B: Aus dem Buffer 1 fest in den Flash-Speicher des Sensors schreiben
						// m_preenrole_id hält die Ziel-ID (0-100), die vorher definiert sein muss
						sprintf_P(m_msg_buffer, PSTR("Storing Model to ID %d."), m_preenrole_id);
						logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
						status = p_finger->storeModel(m_preenrole_id);
						
						if (status == FINGERPRINT_OK) {
							sprintf_P(m_msg_buffer, PSTR("Model %d saved successfully in Flash!"), m_preenrole_id);
							logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
							network.publish(build_topic(MQTT_R503_STATUS_TOPIC, UNIT_TO_PC), m_msg_buffer);
						} else {
							logger.println(TOPIC_GENERIC_INFO, F("Failed to store model in Flash"), COLOR_RED);
							error_to_logger(status);
						}
					} else {
						logger.println(TOPIC_GENERIC_INFO, F("Failed to download template to sensor"), COLOR_RED);
						error_to_logger(status);
					}

					// Puffer nach erfolgreichem/fehlgeschlagenem Schreiben löschen und aufräumen
					free(m_raw_template);
					m_raw_template = nullptr;
					m_download_packet_id = 0;
				} else {
					// Auf das nächste Paket warten
					m_download_packet_id = packet_id + 1;
				}
			} else {
				logger.println(TOPIC_GENERIC_INFO, F("Buffer error or out of memory"), COLOR_RED);
			}
		}
	}
	
	return false; // not for me
}

bool r503::parse(uint8_t* config){
	return cap.parse(config,get_key());
}


uint8_t r503::hexCharToVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0; // Fehlerfall (kein Hex-Zeichen)
}



// Hilfsfunktion zur dynamischen Berechnung des Zeichens
inline char r503::getBase64Char(uint8_t val) {
    if (val < 26) {
        return 'A' + val;             // 0..25 -> 'A'..'Z'
    } else if (val < 52) {
        return 'a' + (val - 26);      // 26..51 -> 'a'..'z'
    } else if (val < 62) {
        return '0' + (val - 52);      // 52..61 -> '0'..'9'
    } else if (val == 62) {
        return '+';
    } else {
        return '/';                   // val == 63
    }
}


void r503::encodeBase64(const uint8_t* input, size_t input_len, char* output) {
    size_t i = 0;
    size_t j = 0;
    uint8_t a, b, c;
    uint32_t combined;

    while (i + 2 < input_len) {
        a = input[i++];
        b = input[i++];
        c = input[i++];

        combined = (a << 16) | (b << 8) | c;

        // Dynamische Generierung statt Array-Lookup
        output[j++] = getBase64Char((combined >> 18) & 0x3F);
        output[j++] = getBase64Char((combined >> 12) & 0x3F);
        output[j++] = getBase64Char((combined >> 6)  & 0x3F);
        output[j++] = getBase64Char(combined         & 0x3F);
    }

    if (i < input_len) {
        a = input[i++];
        b = (i < input_len) ? input[i++] : 0;

        combined = (a << 16) | (b << 8);

        output[j++] = getBase64Char((combined >> 18) & 0x3F);
        output[j++] = getBase64Char((combined >> 12) & 0x3F);
        output[j++] = (i == input_len + 1) ? getBase64Char((combined >> 6) & 0x3F) : '=';
        output[j++] = '=';
    }

    output[j] = '\0';
}

// Hilfsfunktion: Liefert den numerischen Wert eines Base64-Zeichens
inline int r503::base64Val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1; // Padding '=' oder ungültiges Zeichen
}

void r503::decodeBase64(const char* input, uint8_t* output) {
    size_t i = 0;
    size_t j = 0;
    
    // Wir verarbeiten immer 4 Base64-Zeichen zu 3 echten Bytes
    // Für einen 32-Byte-Block läuft das genau 11-mal (33 Bytes, das letzte wird ignoriert/Paddding)
    while (input[i] && input[i] != '=') {
        int v1 = base64Val(input[i++]);
        int v2 = (input[i] && input[i] != '=') ? base64Val(input[i++]) : 0;
        int v3 = (input[i] && input[i] != '=') ? base64Val(input[i++]) : -1;
        int v4 = (input[i] && input[i] != '=') ? base64Val(input[i++]) : -1;

        if (v1 == -1 || v2 == -1) break; // Fehler oder Ende

        // Byte 1 zusammensetzen
        output[j++] = (v1 << 2) | (v2 >> 4);

        // Byte 2 zusammensetzen
        if (v3 != -1) {
            output[j++] = ((v2 & 0x0F) << 4) | (v3 >> 2);
        }
        
        // Byte 3 zusammensetzen
        if (v4 != -1) {
            output[j++] = ((v3 & 0x03) << 6) | v4;
        }
    }
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool r503::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		//m_discovery_topics = malloc(20);
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			m_discovery_pub  = discovery(DOMAIN_BINARY_SENSOR,MQTT_R503_DINGDONG_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_SENSOR,MQTT_R503_STATUS_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_SENSOR,MQTT_R503_CONFIDENCE_TOPIC,UNIT_PERC);
			m_discovery_pub &= discovery(DOMAIN_SENSOR,MQTT_R503_FINGER_ID_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_SWITCH,MQTT_R503_SILENT_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_SWITCH,MQTT_R503_TOUCHRING_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_INPUT_NUMBER,MQTT_R503_PREENROLE_COMMAND_TOPIC,UNIT_NONE);
			m_discovery_pub &= discovery(DOMAIN_INPUT_BUTTON,MQTT_R503_ENROLE_COMMAND_TOPIC,UNIT_NONE);
			return true;
		}
	}
#endif
 	return false;
}
#endif
