#include <cap_audio.h>

// simply the constructor
audio::audio(){
	buffer8b = NULL;
	server = NULL;
	sprintf((char *) key, "AUD");
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
	// either AUD16 for 16 bit or AUD1,2,3,4,5,12,13,14,15 for 8 bit, AUD6..11 are not allowed
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
};

// simply the destructor
audio::~audio(){
	if (buffer8b) {
		delete [] buffer8b;
		buffer8b = NULL;
	}
	if (server) {
		server->close();
		i2s_end(); // i2s was only started when the server is running so we only have to end it now
	}

	pinMode(AMP_ENABLE_PIN, OUTPUT);
	digitalWrite(AMP_ENABLE_PIN, LOW);
	logger.println(TOPIC_GENERIC_INFO, F("audio deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool audio::parse(uint8_t * config){
	return cap.parse_wide(config,get_key(),&bit_mode); // should really only accept AUD8 and AUD16 but hey ..
}

// the will be requested to check if the key is in the config strim
uint8_t * audio::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool audio::init(){
	i2s_begin();
	i2s_set_rate(44100);

	pinMode(AMP_ENABLE_PIN, OUTPUT);
	digitalWrite(AMP_ENABLE_PIN, LOW);

	server = new WiFiServer(AUDIO_PORT);
	server->begin();

	buffer8b = new uint8_t[BUFFER_SIZE];
	logger.println(TOPIC_GENERIC_INFO, F("Audio init"), COLOR_GREEN);
	if (buffer8b) {
		return true;
	}
	return false;
}

// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t audio::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool audio::loop(){
	WiFiClient client = server->available();

	// new client?
	if (client) {
		startStreaming(&client);
		client.stop();
		return true; // i did something
	}
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool audio::intervall_update(uint8_t slot){
	/*if(slot%count_intervall_update()==0){
	 * logger.print(TOPIC_MQTT_PUBLISH, F(""));
	 * dtostrf(audio.getSomething(), 3, 2, m_msg_buffer);
	 * logger.p(F("audio "));
	 * logger.pln(m_msg_buffer);
	 * return network.publish(build_topic(MQTT_audio_TOPIC,UNIT_TO_PC), m_msg_buffer, true);
	 * }*/
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool audio::subscribe(){
	// network.subscribe(build_topic(MQTT_audio_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	// logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_audio_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool audio::receive(uint8_t * p_topic, uint8_t * p_payload){
	/*if (!strcmp((const char *) p_topic, build_topic(MQTT_audio_TOPIC,PC_TO_UNIT))) { // on / off with dimming
	 * logger.println(TOPIC_MQTT, F("received audio command"),COLOR_PURPLE);
	 * // do something
	 * return true;
	 * }*/
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool audio::publish(){
	return false;
}

// aplification ..
int16_t audio::Amplify(int16_t s){
	int32_t v = (s * gainF2P6) >> 6;

	if (v < -32767) return -32767;
	else if (v > 32767) return 32767;
	else return (int16_t) (v & 0xffff);
}

// play the data to the i2s amp
bool audio::ConsumeSample(int16_t sample){
	// fixed24p8_t newSamp = ((int32_t)Amplify(sample)) << 8;
	// mono to stereo
	uint32_t s32 = (sample << 16) & 0xffff0000 | (sample & 0xffff);
	return i2s_write_sample_nb(s32); // If we can't store it, return false.  OTW true
}

// **************************************************
// **************************************************
// **************************************************
inline void audio::startStreaming(WiFiClient * client){
	bufferPtrIn  = 0;
	bufferPtrOut = 0;

	// ===================================================================================
	// fill buffer
	// 16384 byte buffer, 44100 hz = 371 ms playtime
	ultimeout = millis() + 500;
	do {
		// yield();
		if (client->available()) {
			buffer8b[bufferPtrIn] = client->read();
			bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
			ultimeout   = millis() + 500;
		}
	} while ((bufferPtrIn < (BUFFER_SIZE - 1)) && (client->connected()) && (millis() < ultimeout));

	if ((!client->connected()) || (millis() >= ultimeout)) {
		return;
	}

	// ===================================================================================
	digitalWrite(AMP_ENABLE_PIN, LOW); // drive low, to disable pull up
	pinMode(AMP_ENABLE_PIN, INPUT);    // floating to reanable the sample
	// ===================================================================================
	// start playback
	ultimeout = millis() + 500;
	bool play = true;
	while(play){
		if (((bufferPtrIn - bufferPtrOut + BUFFER_SIZE) % BUFFER_SIZE)>=2) {
			// scale down by 4 (>>2) otherwise the output overshoots significantly
			uint16_t t = buffer8b[bufferPtrOut] << 6;
			if(bit_mode == 16){
				t |= buffer8b[(bufferPtrOut+1)%BUFFER_SIZE]>>2;
			}
			// play
			if (ConsumeSample(t)){
				if(bit_mode == 16){
					bufferPtrOut = (bufferPtrOut + 2) % BUFFER_SIZE;
				} else {
					bufferPtrOut = (bufferPtrOut + 1) % BUFFER_SIZE;
				}
			}

			// no timeout, we still have data, playback stops once we didn't have data for 500ms
			ultimeout = millis() + 500;
		} else if( millis() > ultimeout ) {
			play = false; // timeout! can stll overridden by new data
		}

		// new data in wifi rx-buffer?
		if (client->available()) {
			// ring-buffer free?
			if (((bufferPtrIn + 1) % BUFFER_SIZE) != bufferPtrOut) {
				buffer8b[bufferPtrIn] = client->read();
				bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
			}
			play = true;
		}
	}

	// ===================================================================================
	pinMode(AMP_ENABLE_PIN, OUTPUT); // drive it to power down
	digitalWrite(AMP_ENABLE_PIN, LOW);
	// ===================================================================================
} // startStreaming
