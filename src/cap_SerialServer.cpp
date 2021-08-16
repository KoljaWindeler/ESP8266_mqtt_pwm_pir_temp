#ifdef WITH_SERIALSERVER
#include <cap_SerialServer.h>

// simply the constructor
SerialServer::SerialServer(){
	m_port = 1778; // TODO .. make those numbers configurable
	m_rxPin = 15; // D8
	m_txPin = 13; // D7
	m_bufferSize = 128;
};

// simply the destructor
SerialServer::~SerialServer(){
	logger.println(TOPIC_GENERIC_INFO, F("SerialServer deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool SerialServer::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* SerialServer::get_key(){
	return (uint8_t*)"SERSER";
}

// will be callen if the key is part of the config
bool SerialServer::init(){
	m_rxCount = 0;
	m_txCount = 0;
	// wifi server
	ser2netServer = new WiFiServer(m_port);
	ser2netServer->begin();
	// serial interface
	//swSer1 = new SoftwareSerial(m_rxPin, m_txPin, false, m_bufferSize);
	//swSer1->begin(115200);
	logger.enable_serial_trace(false);
	logger.enable_mqtt_trace(true);
	Serial.flush();
	Serial.end();
	delay(50);
	Serial.begin(115200);
	//swSer1->enableIntTx(true);
	logger.println(TOPIC_GENERIC_INFO, F("SerialServer init"), COLOR_GREEN);
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
uint8_t SerialServer::count_intervall_update(){
	return 2;
}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool SerialServer::loop(){
	if (ser2netServer->hasClient()) {
		if (ser2netClient){
			ser2netClient.stop();
		}
		logger.println(TOPIC_GENERIC_INFO, F("Client connected"), COLOR_GREEN);
		ser2netClient = ser2netServer->available();
	}

	if (ser2netClient.connected()) {
		// move data from wifi to serial //
		uint8_t count = ser2netClient.available();
		if (count > 0) {
			//Serial.printf("Data in %i bytes\r\n",count);
			if (count > m_bufferSize) {
				count = m_bufferSize;
			}
			uint8_t net_buf[count];
			uint8_t bytes_read = ser2netClient.read(net_buf, count);
			m_txCount += bytes_read;
			//Serial.write((const uint8_t*)net_buf, bytes_read);
			for(uint8_t i=0; i<bytes_read; i++){
				//swSer1->write(net_buf[i]);
				Serial.write(net_buf[i]);
			}
		}
		// move data from wifi to serial //

		// move data from serial to wifi //
		//count = swSer1->available();
		count = Serial.available();
		if (count > 0) {
			if (count > m_bufferSize) {
				count = m_bufferSize;
			}
			uint8_t net_buf[count];
			m_rxCount += count;
			for(uint8_t i=0; i<count; i++){
				//net_buf[i]=swSer1->read();
				net_buf[i]=Serial.read();
			}
			ser2netClient.write((const uint8_t*)net_buf, count);
			ser2netClient.flush();
		}
		// move data from serial to wifi //
	}
	return false; // i did nothing
}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool SerialServer::intervall_update(uint8_t slot){
	if(slot%count_intervall_update()==0){
		logger.print(TOPIC_MQTT_PUBLISH, F(""));
		sprintf(m_msg_buffer,"%i",m_rxCount);
		m_rxCount = 0;
		logger.p(F("SerialServer Rx Count "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_SSERIALSERVER_RX_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else {
		logger.print(TOPIC_MQTT_PUBLISH, F(""));
		sprintf(m_msg_buffer,"%i",m_txCount);
		m_txCount = 0;
		logger.p(F("SerialServer Tx Count "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_SSERIALSERVER_TX_TOPIC,UNIT_TO_PC), m_msg_buffer);	
	}
	return false;
}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool SerialServer::subscribe(){
//	network.subscribe(build_topic(MQTT_SerialServer_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_SerialServer_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool SerialServer::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_SerialServer_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received SerialServer command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
//bool SerialServer::publish(){
//	return false;
//}
#endif
