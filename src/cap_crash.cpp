#include <cap_crash.h>
#ifdef WITH_CRASH
// simply the constructor
crash::crash(){};

// simply the destructor
crash::~crash(){
	logger.println(TOPIC_GENERIC_INFO, F("crash deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool crash::parse(uint8_t* config){
	//return cap.parse(config,get_key());
	return true;
}

// the will be requested to check if the key is in the config strim
uint8_t* crash::get_key(){
	return (uint8_t*)"CRASH";
}

// will be callen if the key is part of the config
bool crash::init(){
	m_published = false;
	p_crash = new EspSaveCrash(512,512);
	logger.println(TOPIC_GENERIC_INFO, F("CRASH init"), COLOR_GREEN);
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
//uint8_t crash::count_intervall_update(){
//	return 0;
//}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
//bool crash::loop(){
//	return false; // i did nothing
//}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
//bool crash::intervall_update(uint8_t slot){
//	if(slot%count_intervall_update()==0){
//		logger.print(TOPIC_MQTT_PUBLISH, F(""));
//		dtostrf(crash.getSomething(), 3, 2, m_msg_buffer);
//		logger.p(F("crash "));
//		logger.pln(m_msg_buffer);
//		return network.publish(build_topic(MQTT_crash_TOPIC,UNIT_TO_PC), m_msg_buffer);
//	}
//	return false;
//}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool crash::subscribe(){
//	network.subscribe(build_topic(MQTT_crash_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_crash_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool crash::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_crash_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received crash command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool crash::publish(){
	if(m_published){
		return false;
	} else {
		if(p_crash->count()>0){
			sprintf(m_msg_buffer,"%i crashes found, publishing", p_crash->count());
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_RED);

			// Note that 'EEPROM.begin' method is reserving a RAM buffer
			// The buffer size is SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE
			EEPROM.begin(512 + 512);
			byte crashCounter = EEPROM.read(512 + SAVE_CRASH_COUNTER);
			
			if(crashCounter>1){ // ignore uninitialized EEPROM area (default to 0xFF), there can be only 1 crash at most
				crashCounter=0;
			}
			int16_t readFrom = 512 + SAVE_CRASH_DATA_SETS;
			for (byte k = 0; k < crashCounter; k++)
			{
				uint32_t crashTime;
				EEPROM.get(readFrom + SAVE_CRASH_CRASH_TIME, crashTime);
				sprintf(m_msg_buffer,"Crash # %d at %ld ms", k + 1, (long) crashTime);
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);

				sprintf(m_msg_buffer,"Restart reason: %d", EEPROM.read(readFrom + SAVE_CRASH_RESTART_REASON));
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);
				sprintf(m_msg_buffer,"Exception cause: %d", EEPROM.read(readFrom + SAVE_CRASH_EXCEPTION_CAUSE));
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);

				uint32_t epc1, epc2, epc3, excvaddr, depc;
				EEPROM.get(readFrom + SAVE_CRASH_EPC1, epc1);
				EEPROM.get(readFrom + SAVE_CRASH_EPC2, epc2);
				EEPROM.get(readFrom + SAVE_CRASH_EPC3, epc3);
				EEPROM.get(readFrom + SAVE_CRASH_EXCVADDR, excvaddr);
				EEPROM.get(readFrom + SAVE_CRASH_DEPC, depc);
				sprintf(m_msg_buffer,"epc1=0x%08x epc2=0x%08x epc3=0x%08x", epc1, epc2, epc3);
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);
				sprintf(m_msg_buffer,"excvaddr=0x%08x depc=0x%08x",excvaddr, depc);
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);

				uint32_t stackStart, stackEnd;
				EEPROM.get(readFrom + SAVE_CRASH_STACK_START, stackStart);
				EEPROM.get(readFrom + SAVE_CRASH_STACK_END, stackEnd);
				sprintf(m_msg_buffer,">>>stack>>>");
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);

				int16_t currentAddress = readFrom + SAVE_CRASH_STACK_TRACE;
				int16_t stackLength = stackEnd - stackStart;
				uint32_t stackTrace;
				for (int16_t i = 0; i < stackLength; i += 0x10){
					sprintf(m_msg_buffer,"%08x: ", stackStart + i);  // 10 < 60
					// m_msg_buffer can hold 60 chars, lets fill them here
					for (byte j = 0; j < 4; j++){
						EEPROM.get(currentAddress, stackTrace);
						sprintf(m_msg_buffer+10+j*9,"%08x ", stackTrace); // 10+4*9 = 46 < 60
						currentAddress += 4;
						if (currentAddress - 512 > 512){
							i = stackLength;
							break;
							//outputDev.println("\nIncomplete stack trace saved!");
							//goto eepromSpaceEnd;
						}
					}
					network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), m_msg_buffer);
				}
				network.publish(build_topic(MQTT_CRASH_TOPIC,UNIT_TO_PC), "<<<stack<<<");
				readFrom = readFrom + SAVE_CRASH_STACK_TRACE + stackLength;
			}
			int16_t writeFrom;
			EEPROM.get(512 + SAVE_CRASH_WRITE_FROM, writeFrom);
			EEPROM.end();

			// clear eeprom as we've alread publish the info
			p_crash->clear();
		} else {
			logger.println(TOPIC_GENERIC_INFO, F("0 Crashes found"), COLOR_GREEN);
		}
		m_published = true;
	};
	return false;
}
#endif
