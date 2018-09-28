#include <cap_audio.h>

// simply the constructor
audio::audio(){
	buffer8b  = NULL;
	server    = NULL;
	connected = false;
	sprintf((char *) key, "AUD");
	SetGain(0.35);
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
	return cap.parse_wide(config, get_key(), &bit_mode); // should really only accept AUD8 and AUD16 but hey ..
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
	logger.print(TOPIC_GENERIC_INFO, F("Audio init "), COLOR_GREEN);
	sprintf(m_msg_buffer,"%i bit",bit_mode);
	logger.pln(m_msg_buffer);
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
	run_noninterrupted = true; // get high priority

	if (!connected) {
		client = server->available();
		if(client.connected()){
			logger.println(TOPIC_GENERIC_INFO, F("New audio client"), COLOR_GREEN);
		}
	}

	// new client?
	if (client.connected()) {
		if (!connected) {
			bufferPtrIn  = 0;
			bufferPtrOut = 0;
			// ===================================================================================
			// fill buffer
			// 16384 byte buffer, 44100 hz = 371 ms playtime
			ultimeout = millis() + 500;
			do {
				// yield();
				if (client.available()) {
					buffer8b[bufferPtrIn] = client.read();
					bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
					ultimeout   = millis() + 500;
				}
			} while ((bufferPtrIn < (BUFFER_SIZE - 1)) && (client.connected()) && (millis() < ultimeout));

			if ((!client.connected()) || (millis() >= ultimeout)) {
				logger.println(TOPIC_GENERIC_INFO, F("Audio buffering failed"), COLOR_RED);
				return false;
			}
			logger.println(TOPIC_GENERIC_INFO, F("Audio starts playing"), COLOR_GREEN);
			// still running, prepare playback
			// ===================================================================================
			digitalWrite(AMP_ENABLE_PIN, LOW); // drive low, to disable pull up
			pinMode(AMP_ENABLE_PIN, INPUT);    // floating to reanable the sample
			// ===================================================================================
			ultimeout = millis() + 500;
			connected = true;
			samples_played = 0;
			// start playback
		}

		if (connected) {
			// playback
			if (((bufferPtrIn - bufferPtrOut + BUFFER_SIZE) % BUFFER_SIZE) >= 2) {
				// scale down by 4 (>>2) otherwise the output overshoots significantly
				uint16_t t = Amplify(buffer8b[bufferPtrOut]) << 6;
				if (bit_mode == 16) {
					t |= Amplify(buffer8b[(bufferPtrOut + 1) % BUFFER_SIZE]) >> 2;
				}
				// play
				uint32_t s32 = (t << 16) & 0xffff0000 | (t & 0xffff);

				if (i2s_write_sample_nb(s32)) { // If we can't store it, return false.  OTW true
					run_noninterrupted = false; // our chance, i2s just received new samples, see if we have to publish something
					if (bit_mode == 16) {
						bufferPtrOut = (bufferPtrOut + 2) % BUFFER_SIZE;
					} else {
						bufferPtrOut = (bufferPtrOut + 1) % BUFFER_SIZE;
					}
				}
				// no timeout, we still have data, playback stops once we didn't have data for 500ms
				ultimeout = millis() + 500;
			}
			// timeout! can stll overridden by new data
			else if (millis() > ultimeout) {
				connected = false;
				logger.println(TOPIC_GENERIC_INFO, F("Audio timeout"), COLOR_RED);
			}

			/*
			samples_played++;
			if(samples_played>5000){
				samples_played=0;
				Serial.printf("%i\r\n",((bufferPtrIn - bufferPtrOut + BUFFER_SIZE) % BUFFER_SIZE));
			}
			*/

			// read new data to buffer
			for(uint8_t s=0; s<bit_mode; s+=8){
				if (client.available()) {
					// ring-buffer free?
					if (((bufferPtrIn + 3) % BUFFER_SIZE) != bufferPtrOut) {
						buffer8b[bufferPtrIn] = client.read();
						bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
					}
					connected = true;
				}
			}

			// on disconnect
			if (!connected ) {
				shutdown();
			}
		}
		return run_noninterrupted; // i played music leave me running
	} else   {
		if(connected){
			shutdown();
		}
		// nnope, did nothing, go on
		return false;
	}
} // loop

// shut the AMP down via PIN, fastest and Plopp avoiding
void audio::shutdown(){
	logger.println(TOPIC_GENERIC_INFO, F("Audio shutting down"), COLOR_RED);
	// ===================================================================================
	pinMode(AMP_ENABLE_PIN, OUTPUT); // drive it to power down
	digitalWrite(AMP_ENABLE_PIN, LOW);
	// ===================================================================================
	client.stop();
	connected = false;
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
	network.subscribe(build_topic(MQTT_AUDIO_VOL_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_AUDIO_VOL_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool audio::receive(uint8_t * p_topic, uint8_t * p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_AUDIO_VOL_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		int vol_precent = atoi((const char *)p_payload);
		logger.print(TOPIC_MQTT, F("received audio volume command: "),COLOR_PURPLE);
		sprintf(m_msg_buffer,"%i%%",vol_precent);
		logger.pln(m_msg_buffer);
		SetGain(((float)vol_precent)/100);
	 	return true;
	 }
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool audio::publish(){
	return false;
}

// some sort of amplification concept
bool audio::SetGain(float f) {
	if (f>4.0){
		f = 4.0;
	}
	if (f<0.0){
		f=0.0;
	}
	gainF2P6 = (uint8_t)(f*(1<<6));
	return true;
}

// amplification ..
int16_t audio::Amplify(int16_t s){
	int32_t v = (s * gainF2P6) >> 6;

	if (v < -32767) return -32767;
	else if (v > 32767) return 32767;
	else return (int16_t) (v & 0xffff);
}
