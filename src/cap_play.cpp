#include <cap_play.h>
#ifdef WITH_PLAY
// simply the constructor
play::play(){
	buffer8b   = NULL;
	tcp_server = NULL;
	amp_active = false;
	client_connected  = false;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
	// either AUD16 for 16 bit or AUD1,2,3,4,5,12,13,14,15 for 8 bit, AUD6..11 are not allowed
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! //
};

// simply the destructor
play::~play(){
	if (buffer8b) {
		delete [] buffer8b;
		buffer8b = NULL;
	}
	if (tcp_server) {
		tcp_server->close();
		i2s_end(); // i2s was only started when the tcp_server is running so we only have to end it now
		power_amp(false); // power amp down
	}

	logger.println(TOPIC_GENERIC_INFO, F("play deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool play::parse(uint8_t * config){
	if (cap.parse_wide(config, get_key(), &bit_mode)) {
		// should really only accept AUD8 and AUD16 but hey ..
		if (bit_mode != 16) {
			bit_mode = 8;
		}
		return true;
	}
}


// the will be requested to check if the key is in the config strim
uint8_t * play::get_key(){
	return (uint8_t*)"PLY";
}

// will be callen if the key is part of the config
bool play::init(){
	SetGain(0.35);
	i2s_begin();
	i2s_set_rate(44100); // default, can be change via mqtt
	power_amp(false); // amp power down state

	tcp_server = new WiFiServer(PLAY_PORT);
	tcp_server->begin();

	buffer8b = new uint8_t[BUFFER_SIZE];
	if (buffer8b) {
		logger.print(TOPIC_GENERIC_INFO, F("play init "), COLOR_GREEN);
		sprintf(m_msg_buffer, "%i bit", bit_mode);
		logger.pln(m_msg_buffer);
		return true;
	}
	logger.println(TOPIC_GENERIC_INFO, F("play init failed"), COLOR_RED);
	return false;
}


// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool play::loop(){
	run_noninterrupted = true; // get high priority

	// new tcp_client?
	if (!client_connected) {
		tcp_socket = tcp_server->available();
		if (tcp_socket.connected()) {
			logger.println(TOPIC_GENERIC_INFO, F("(PLY) New tcp_client"), COLOR_GREEN);
			last_data_in = millis();
		}
	}

	if (tcp_socket.connected()) { // timeout default: 1000ms
		if (!client_connected) { // if client is connected now, but the amp is off or the client wasn't connected before
			bufferPtrIn  = 0;
			bufferPtrOut = 0;
			// ===================================================================================
			// fill buffer
			// 16384 byte buffer, 44100 hz = 371 ms playtime
			ultimeout = millis() + 500;
			// yield();
			do {
				if (tcp_socket.available()) {
					buffer8b[bufferPtrIn] = tcp_socket.read();
					bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
					ultimeout   = millis() + 500;
				}
			} while ((bufferPtrIn < (BUFFER_SIZE - 1)) && (tcp_socket.connected()) && (millis() < ultimeout));

			// error handling
			if ((!tcp_socket.connected()) || (millis() >= ultimeout)) {
				logger.println(TOPIC_GENERIC_INFO, F("(PLY) full buffering failed"), COLOR_RED);
				//return false;
			}

			logger.println(TOPIC_GENERIC_INFO, F("(PLY) start playing"), COLOR_GREEN);
			// still running, prepare playback
			// ===================================================================================
			power_amp(true); // power up
			// ===================================================================================
			ultimeout      = millis() + 500;
			last_data_in   = millis();
			client_connected      = true;
			// samples_played = 0;
			// start playback
		}

		if (client_connected) {
			// playback -- keep playing the same sample on buffer underrun up to 1.5sec
			// scale down by 4 (>>2) otherwise the output overshoots significantly
			if(amp_active){
				uint16_t t = Amplify(buffer8b[bufferPtrOut]) << 6;
				if (bit_mode == 16) {
					t |= Amplify(buffer8b[(bufferPtrOut + 1) % BUFFER_SIZE]) >> 2;
				}
				// play
				uint32_t s32 = (t << 16) & 0xffff0000 | (t & 0xffff);

				if (i2s_write_sample_nb(s32)) { // If we can't store it, return false.  OTW true
					//run_noninterrupted = false;    // our chance, i2s just received new samples, see if we have to publish something
					if (((bufferPtrIn - bufferPtrOut + BUFFER_SIZE) % BUFFER_SIZE) >= 2) { // not close to buffer end, step forward
						if (bit_mode == 16) {
							bufferPtrOut = (bufferPtrOut + 2) % BUFFER_SIZE;
						} else {
							bufferPtrOut = (bufferPtrOut + 1) % BUFFER_SIZE;
						}
						ultimeout = millis() + 1500; // we still have data, no timeout
					}
				}

				// timeout, no new data. Not a disconnect .. neccessarly.
				if (millis() > ultimeout) {
					power_amp(false); // power down
					logger.println(TOPIC_GENERIC_INFO, F("(PLY) timeout"), COLOR_RED);
				}
			}

			/*
			 * samples_played++;
			 * if(samples_played>5000){
			 * samples_played=0;
			 * Serial.printf("%i\r\n",((bufferPtrIn - bufferPtrOut + BUFFER_SIZE) % BUFFER_SIZE));
			 * }
			 */

			// read new data to buffer
			for (uint8_t s = 0; s < bit_mode; s += 8) {
				if (tcp_socket.available()>1) {
					last_data_in   = millis();
					// ring-buffer free?
					if (((bufferPtrIn + 3) % BUFFER_SIZE) != bufferPtrOut) {
						buffer8b[bufferPtrIn] = tcp_socket.read();
						bufferPtrIn = (bufferPtrIn + 1) % BUFFER_SIZE;
					}
					if(!amp_active){ // re-enable amp if there are still more data available
						power_amp(true);
					}
				} else if (tcp_socket.available()==1) { // single sample, just a keep alive
					//Serial.println(".");
					last_data_in   = millis();
					tcp_socket.read();
				}
			}

			// disconnect
			if(millis()-last_data_in > 1500){
				handle_client_disconnect();
			}
		}
		return run_noninterrupted; // i played music leave me running
	} else if (client_connected) {
		handle_client_disconnect();
	}
	return false; // catch
} // loop


// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
// bool play::intervall_update(uint8_t slot){
// 	/*if(slot%count_intervall_update()==0){
// 	 * logger.print(TOPIC_MQTT_PUBLISH, F(""));
// 	 * dtostrf(play.getSomething(), 3, 2, m_msg_buffer);
// 	 * logger.p(F("play "));
// 	 * logger.pln(m_msg_buffer);
// 	 * return network.publish(build_topic(MQTT_play_TOPIC,UNIT_TO_PC), m_msg_buffer, true);
// 	 * }*/
// 	return false;
// }

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool play::subscribe(){
	network.subscribe(build_topic(MQTT_play_VOL_TOPIC, PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_play_VOL_TOPIC, PC_TO_UNIT), COLOR_GREEN);
	network.subscribe(build_topic(MQTT_play_SR_TOPIC, PC_TO_UNIT)); // simple rainbow  topic
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_play_SR_TOPIC, PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool play::receive(uint8_t * p_topic, uint8_t * p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_play_VOL_TOPIC, PC_TO_UNIT))) { // on / off with dimming
		int vol_precent = atoi((const char *) p_payload);
		logger.print(TOPIC_MQTT, F("received play volume command: "), COLOR_PURPLE);
		sprintf(m_msg_buffer, "%i%%", vol_precent);
		logger.pln(m_msg_buffer);
		SetGain(((float) vol_precent) / 100);
		return true;
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_play_SR_TOPIC, PC_TO_UNIT)))    { // on / off with dimming
		uint16_t sample_rate = atoi((const char *) p_payload);
		logger.print(TOPIC_MQTT, F("received sample rate command: "), COLOR_PURPLE);
		sprintf(m_msg_buffer, "%i", sample_rate);
		logger.pln(m_msg_buffer);
		i2s_set_rate(sample_rate);
		return true;
	}
	return false; // not for me
}

// turn amplifier on / off
void play::power_amp(bool status){
	if(status){
		digitalWrite(AMP_ENABLE_PIN, LOW); // drive low, to disable pull up
		pinMode(AMP_ENABLE_PIN, INPUT);    // floating to reanable the sample
		logger.println(TOPIC_GENERIC_INFO, F("(PLY) amp power up"), COLOR_GREEN);
	} else {
		pinMode(AMP_ENABLE_PIN, OUTPUT); // drive it to power down
		digitalWrite(AMP_ENABLE_PIN, LOW);
		logger.println(TOPIC_GENERIC_INFO, F("(PLY) amp shutting down"), COLOR_RED);
	}
	amp_active = status;
}

void play::handle_client_disconnect(){
	power_amp(false); // power down
	client_connected = false;
	logger.println(TOPIC_GENERIC_INFO, F("(PLY) tcp_client disconnect"), COLOR_YELLOW);
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
// bool play::publish(){
// 	return false;
// }

// some sort of amplification concept
bool play::SetGain(float f){
	if (f > 4.0) {
		f = 4.0;
	}
	if (f < 0.0) {
		f = 0.0;
	}
	gainF2P6 = (uint8_t) (f * (1 << 6));
	return true;
}

// amplification ..
int16_t play::Amplify(int16_t s){
	int32_t v = (s * gainF2P6) >> 6;

	if (v < -32767) return -32767;
	else if (v > 32767) return 32767;
	else return (int16_t) (v & 0xffff);
}
#endif
