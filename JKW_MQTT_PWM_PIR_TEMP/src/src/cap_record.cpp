#include <cap_record.h>

// simply the constructor
record::record(){
	adc_buf = NULL;
	sprintf((char*)key,"REC");
};

// simply the destructor
record::~record(){
	if(adc_buf){
		delete [] adc_buf;
		adc_buf = NULL;
	}
	logger.println(TOPIC_GENERIC_INFO, F("record deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool record::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* record::get_key(){
	return key;
}

void ICACHE_RAM_ATTR sample_isr_rec(void){
	if(p_rec){
		((record*)p_rec)->sample_isr();
	}
}

// will be callen if the key is part of the config
bool record::init(){
	adc_buf = new uint16_t[2*REC_BUFFER_SIZE];

	silence_value = 2048; // computed as an exponential moving average of the signal
	envelope_threshold = 150; // envelope threshold to trigger data sending
	send_sound_util = 0; // date until sound transmission ends after an envelope threshold has triggered sound transmission
	enable_highpass_filter = 0;

	spiBegin();
	timer1_isr_init();
	timer1_attachInterrupt(sample_isr_rec);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
	timer1_write(clockCyclesPerMicrosecond() / 16 * 50); //80us = 12.5kHz sampling freq

	//udp.begin(TARGET_PORT);

	logger.println(TOPIC_GENERIC_INFO, F("HLW init"), COLOR_GREEN);
	return true;
}

// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t record::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool record::loop(){
	if (send_samples_now) {
		/* We're ready to send a buffer of samples over wifi. Decide if it has to happen or not,
		   that is, if the sound level is above a certain threshold. */

		// Update silence and envelope computations
		uint16_t number_of_samples = REC_BUFFER_SIZE;
		int32_t accum_silence = 0;
		int32_t envelope_value = 0;

		int32_t now = millis();
		uint8_t *writeptr = (uint8_t *)(&adc_buf[(!current_adc_buf*REC_BUFFER_SIZE)+0]);
		uint16_t *readptr;
		uint16_t last = 0;
		for (unsigned int i = 0; i < number_of_samples; i++) {
			readptr = &adc_buf[(!current_adc_buf*REC_BUFFER_SIZE)+i];
			int32_t val = *readptr;
			int32_t rectified;

			/*if (enable_highpass_filter) {
				*readptr = filterloop(val) + 2048;
				val = *readptr;
			}*/

			rectified = abs(val - silence_value);

			accum_silence += val;
			envelope_value += rectified;

			// delta7-compress the data
			writeptr = delta7_sample(last, readptr, writeptr);
			last = val;
		}
		accum_silence /= number_of_samples;
		envelope_value /= number_of_samples;
		silence_value = (SILENCE_EMA_WEIGHT * silence_value + accum_silence) / (SILENCE_EMA_WEIGHT + 1);
		envelope_value = envelope_value;

		if (envelope_value > envelope_threshold) {
			send_sound_util = millis() + 15000;
		}

		if (millis() < send_sound_util) {
			Serial.print("sending ");
			udp.beginPacket(IPAddress(192,168,2,59), 5523);
			uint16_t len = writeptr - (uint8_t *)&adc_buf[(!current_adc_buf*REC_BUFFER_SIZE)+0];
			Serial.println(len);
			udp.write((const uint8_t *)(&adc_buf[(!current_adc_buf*REC_BUFFER_SIZE)+0]), len);
			udp.endPacket();
		}
		send_samples_now = 0;
		Serial.print("Silence val "); Serial.print(silence_value); Serial.print(" envelope val "); Serial.print(envelope_value);
		Serial.print("delay "); Serial.print(millis() - now);
		Serial.println("");
	}
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool record::intervall_update(uint8_t slot){
	/*
	if(slot%count_intervall_update()==0){
		logger.print(TOPIC_MQTT_PUBLISH, F(""));
		dtostrf(record.getSomething(), 3, 2, m_msg_buffer);
		logger.p(F("record "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_record_TOPIC,UNIT_TO_PC), m_msg_buffer, true);
	}
	*/
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool record::subscribe(){
	//network.subscribe(build_topic(MQTT_record_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	//logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_record_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool record::receive(uint8_t* p_topic, uint8_t* p_payload){
	/*if (!strcmp((const char *) p_topic, build_topic(MQTT_record_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received record command"),COLOR_PURPLE);
		// do something
		return true;
	}*/
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool record::publish(){
	return false;
}

inline void record::setDataBits(uint16_t bits) {
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    bits--;
    SPI1U1 = ((SPI1U1 & mask) | ((bits << SPILMOSI) | (bits << SPILMISO)));
}

void record::spiBegin(void)
{
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  SPI.setHwCs(1);
  setDataBits(16);
}


/* SPI code based on the SPI library */
inline ICACHE_RAM_ATTR uint16_t record::transfer16(void) {
	union {
		uint16_t val;
		struct {
			uint8_t lsb;
			uint8_t msb;
		};
	} out;


	// Transfer 16 bits at once, leaving HW CS low for the whole 16 bits
	while(SPI1CMD & SPIBUSY) {}
	SPI1W0 = 0;
	SPI1CMD |= SPIBUSY;
	while(SPI1CMD & SPIBUSY) {}

	/* Follow MCP3201's datasheet: return value looks like this:
	xxxBA987 65432101
	We want
	76543210 0000BA98
	So swap the bytes, select 12 bits starting at bit 1, and shift right by one.
	*/

	out.val = SPI1W0 & 0xFFFF;
	uint8_t tmp = out.msb;
	out.msb = out.lsb;
	out.lsb = tmp;

	out.val &= (0x0FFF << 1);
	out.val >>= 1;
	return out.val;
}



void ICACHE_RAM_ATTR record::sample_isr(void){
	uint16_t val;

	// Read a sample from ADC
	val = transfer16();
	adc_buf[(current_adc_buf*REC_BUFFER_SIZE)+adc_buf_pos] = val & 0xFFF;
	adc_buf_pos++;

	// If the buffer is full, signal it's ready to be sent and switch to the other one
	if (adc_buf_pos > REC_BUFFER_SIZE) {
		adc_buf_pos = 0;
		current_adc_buf = !current_adc_buf;
		send_samples_now = 1;
	}
}


uint8_t* record::delta7_sample(uint16_t last, uint16_t *readptr, uint8_t *writeptr){
	const uint8_t lowbyte1 = *((uint8_t *)readptr);
	const uint8_t highbyte1 = *((uint8_t *)readptr+1);
	const uint16_t val = *readptr;

	const int32_t diff = val - last;
	if (diff > -64 && diff < 64) {
		// 7bit delta possible
		// Encode the delta as "sign and magnitude" format.
		// CSMMMMMM (compressed signed magnitude^6)
		int8_t out = 0x80 | ((diff < 0) ? 0x40 : 0x0) | abs(diff);
		*writeptr++ = out;
	} else {
		// 7bit delta impossible, output as-is
		*writeptr++ = highbyte1;
		*writeptr++ = lowbyte1;
	}

	return writeptr;
}
