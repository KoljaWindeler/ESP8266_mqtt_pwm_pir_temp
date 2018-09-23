#include <cap_audio.h>

// simply the constructor
audio::audio(){
	buffer8b = NULL;
	sprintf((char *) key, "AUD");
};

// simply the destructor
audio::~audio(){
	if (buffer8b) {
		delete [] buffer8b;
		buffer8b = NULL;
	}
	server->close();
	i2s_end();
	digitalWrite(AMP_ENABLE_PIN, LOW);
	logger.println(TOPIC_GENERIC_INFO, F("audio deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool audio::parse(uint8_t * config){
	return cap.parse(config, get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t * audio::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool audio::init(){
	/*
	 * timer0_isr_init();
	 * timer0_attachInterrupt(statusLED_ISR);
	 * timer0_write(ESP.getCycleCount() + 160000000);
	 * interrupts();
	 */
	i2s_begin();
	// gainF2P6 = (uint8_t)(1.0*(1<<6));
	// parameter 96
	i2s_set_rate((44100 * 96 / 32));
	lastSamp = 0;
	cumErr   = 0;
	// neu //

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
		return true; // i did nothing
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

// **************************************************
// **************************************************
// **************************************************
// ctrl = 0 : reset the PWM
// ctrl = 1 : normal playback
// ctrl = 2 : direct 8-bit value output
//
inline void audio::doPWM(uint8_t ctrl, uint8_t value8b){
	static uint8_t sample  = 0;
	static uint8_t dWordNr = 0;
	static uint32_t i2sbuf32b[4];
	static uint8_t prng;
	uint8_t shift;

	// reset
	if (ctrl == PWM_RESET) {
		dWordNr      = 0;
		sample       = 0;
		i2sbuf32b[0] = 0;
		i2sbuf32b[1] = 0;
		i2sbuf32b[2] = 0;
		i2sbuf32b[3] = 0;
		return;
	}

	// playback
	if (i2s_write_sample_nb(i2sbuf32b[dWordNr])) {
		dWordNr = (dWordNr + 1) & 0x03;

		// previous 4 DWORDS written?
		if (dWordNr == 0x00) {
			yield();

			// normal playback
			if (ctrl == PWM_NORMAL) {
				// new data in ring-buffer?
				if (bufferPtrOut != bufferPtrIn) {
					sample       = buffer8b[bufferPtrOut];
					bufferPtrOut = (bufferPtrOut + 1) & (BUFFER_SIZE - 1);
				}
			} else if (ctrl == PWM_DIRECT) {
				// direct output mode
				sample = value8b;
			}

			shift = sample >> 1;

			if (!(sample & 0x01)) {
				shift -= (prng & 0x01); // subtract 0 or 1 from "shift" for dithering
				prng  ^= 0x01;
			}

			yield();

			shift = 0x80 - shift; // inverse shift

			if (shift < 0x20) {
				i2sbuf32b[0] = 0xFFFFFFFF >> shift;
				i2sbuf32b[1] = 0xFFFFFFFF;
				i2sbuf32b[2] = 0xFFFFFFFF;
				i2sbuf32b[3] = 0xFFFFFFFF;
			} else if (shift < 0x40) {
				i2sbuf32b[0] = 0x00000000;
				i2sbuf32b[1] = 0xFFFFFFFF >> (shift - 0x20);
				i2sbuf32b[2] = 0xFFFFFFFF;
				i2sbuf32b[3] = 0xFFFFFFFF;
			} else if (shift < 0x60) {
				i2sbuf32b[0] = 0x00000000;
				i2sbuf32b[1] = 0x00000000;
				i2sbuf32b[2] = 0xFFFFFFFF >> (shift - 0x40);
				i2sbuf32b[3] = 0xFFFFFFFF;
			} else if (shift < 0x80) {
				i2sbuf32b[0] = 0x00000000;
				i2sbuf32b[1] = 0x00000000;
				i2sbuf32b[2] = 0x00000000;
				i2sbuf32b[3] = 0xFFFFFFFF >> (shift - 0x60);
			} else {
				i2sbuf32b[0] = 0x00000000;
				i2sbuf32b[1] = 0x00000000;
				i2sbuf32b[2] = 0x00000000;
				i2sbuf32b[3] = 0x00000000;
			}
		}
	}
} // doPWM

// aplification ..
int16_t audio::Amplify(int16_t s){
	int32_t v = (s * gainF2P6) >> 6;
	if (v < -32767) return -32767;
	else if (v > 32767) return 32767;
	else return (int16_t) (v & 0xffff);
}

bool audio::ConsumeSample(int16_t sample){
	// Make delta-sigma filled buffer
	uint32_t dsBuff[8];
	// Not shift 8 because addition takes care of one mult x 2

	fixed24p8_t newSamp = ((int32_t)Amplify(sample)) << 8; //( (int32_t) sample ) << 8;

	int oversample32 = 96 / 32;
	// How much the comparison signal changes each oversample step
	fixed24p8_t diffPerStep = (newSamp - lastSamp) >> (4 + oversample32);

	// Don't need lastSamp anymore, store this one for next round
	lastSamp = newSamp;

	uint32_t bits; // The bits we convert the sample into, MSB to go on the wire first
	for (int j = 0; j < oversample32; j++) {
		bits = 0;
		// loop over all bits
		for (int i = 32; i > 0; i--) {
			bits = bits << 1;
			if (cumErr < 0) {
				bits   |= 1; // set if difference is neg
				cumErr += fixedPosValue - newSamp;
			} else {
				// Bits[0] = 0 handled already by left shift
				cumErr -= fixedPosValue + newSamp;
			}
			newSamp += diffPerStep; // Move the reference signal towards destination
		}
		dsBuff[j] = bits; // bit is sample to play
	}

	// DeltaSigma(ms, dsBuff);
	yield();
	// Either send complete pulse stream or nothing
	if (!i2s_write_sample_nb(dsBuff[0])) return false;  // No room at the inn
	//
	yield();
	// At this point we've sent in first of possibly 8 32-bits, need to send
	// remaining ones even if they block.
	for (int i = 32; i < 96; i += 32){
		i2s_write_sample(dsBuff[i / 32]);
	}
	return true;
} // ConsumeSample

// **************************************************
// **************************************************
// **************************************************
void audio::rampPWM(uint8_t direction, uint16_t target){
	if (direction == UP) {
		for (uint8_t value = 0; value < target; value++) {
			for (uint8_t dl = 0; dl <= 250; dl++) {
				doPWM(PWM_DIRECT, value);
			}
		}
	} else {
		for (uint8_t value = target; value > 0; value--) {
			for (uint8_t dl = 0; dl <= 250; dl++) {
				doPWM(PWM_DIRECT, value);
			}
		}
	}
}

// **************************************************
// **************************************************
// **************************************************
inline void audio::startStreaming(WiFiClient * client){
	uint8_t i;
	uint8_t dl;

	bufferPtrIn  = 0;
	bufferPtrOut = 0;


	// ===================================================================================
	// fill buffer

	ultimeout = millis() + 500;
	do {
		// yield();
		if (client->available()) {
			buffer8b[bufferPtrIn] = client->read();
			bufferPtrIn = (bufferPtrIn + 1) & (BUFFER_SIZE - 1);
			ultimeout   = millis() + 500;
		}
	} while ((bufferPtrIn < (BUFFER_SIZE - 1)) && (client->connected()) && (millis() < ultimeout));

	if ((!client->connected()) || (millis() >= ultimeout))
		return;

	// ===================================================================================
	// ramp-up PWM to 50% (=Vspeaker/2) to avoid "blops"
	rampPWM(UP, buffer8b[bufferPtrIn]);
	digitalWrite(AMP_ENABLE_PIN, HIGH);
	// ===================================================================================
	// start playback
	ultimeout = millis() + 500;
	do {
		// doPWM(PWM_NORMAL,0);
		if (bufferPtrOut != bufferPtrIn) {
			if (ConsumeSample(buffer8b[bufferPtrOut] << 6)) {
				bufferPtrOut = (bufferPtrOut + 1) % BUFFER_SIZE;
			}
			// if(ConsumeSample(((uint16_t)buffer8b[bufferPtrOut])<<8|| ((uint16_t)buffer8b[(bufferPtrOut+1)%BUFFER_SIZE]))){
			//	bufferPtrOut = (bufferPtrOut + 2) % BUFFER_SIZE;
			// }
		}

		// new data in wifi rx-buffer?
		if (client->available()) {
			// ring-buffer free?
			if (((bufferPtrIn + 1) & (BUFFER_SIZE - 1)) != bufferPtrOut) {
				buffer8b[bufferPtrIn] = client->read();
				bufferPtrIn = (bufferPtrIn + 1) & (BUFFER_SIZE - 1);
			}
		}

		if (bufferPtrOut != bufferPtrIn) {
			ultimeout = millis() + 500;
		} else {
			// no data left, exit
			break;
		}
	} while (client->available() || (millis() < ultimeout) || (bufferPtrOut != bufferPtrIn));
	// ===================================================================================

	// disabling the amplifier does not "plop", so it's better to do it before ramping down
	digitalWrite(AMP_ENABLE_PIN, LOW);
	// ramp-down PWM to 0% (=0Vdc) to avoid "blops"
	rampPWM(DOWN, buffer8b[(bufferPtrOut + BUFFER_SIZE - 1) % BUFFER_SIZE]);
} // startStreaming
