#include <cap_ads1015.h>

// simply the constructor
ads1015::ads1015(){
	m_i2cAddress = ADS1015_ADDRESS; // assume ADR == GND 
	m_conversionDelay = ADS1115_CONVERSIONDELAY;
	m_gain = ADS1015_REG_CONFIG_PGA_4_096V; // see init()
	m_acdc_mode = ADS_DC_MODE; // see parse ()
	m_discovery_pub = false;
};

// simply the destructor
ads1015::~ads1015(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		for(uint8_t i=0; i<4; i++){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_ADS_CHi_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
			logger.print(TOPIC_MQTT_PUBLISH, F("Erasing ADS config "), COLOR_YELLOW);
			logger.pln(t);
			network.publish(t,(char*)"");
			delete[] t;
		}
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("ads1015 deleted"), COLOR_YELLOW);
};

// the will be requested to check if the key is in the config strim
uint8_t* ads1015::get_key(){
	return (uint8_t*)"ADS";
}

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool ads1015::parse(uint8_t* config){
	bool ret = false;
	if(cap.parse(config,(uint8_t*)"ADS_AC")){
		ret = true;
		m_acdc_mode = ADS_AC_MODE;
	} else if(cap.parse(config,get_key())){
		ret = true;
		m_acdc_mode = ADS_DC_MODE;
	} else if(cap.parse(config,(uint8_t*)"ADS_DC")){
		ret = true;
		m_acdc_mode = ADS_DC_MODE;
	}
	return ret;
}

// will be callen if the key is part of the config
bool ads1015::init(){
	Wire.begin(ADS_SDA_PIN,ADS_SCL_PIN); 
	setGain(ADS1015_REG_CONFIG_PGA_4_096V);
	logger.print(TOPIC_GENERIC_INFO, F("ADS init, mode: "), COLOR_GREEN);
	if(m_acdc_mode==ADS_AC_MODE){
		logger.pln(F("AC"));
	} else {
		logger.pln(F("DC"));
	}
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
uint8_t ads1015::count_intervall_update(){
	return 4*3;
}

// override-methode, only implement when needed
// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
//bool ads1015::loop(){
//	return false; // i did nothing
//}

// override-methode, only implement when needed
// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool ads1015::intervall_update(uint8_t slot){
	float amp_volt;
	if(m_acdc_mode == ADS_AC_MODE){
		configureADC_continues(slot%4);
		amp_volt = readADC_continues_ac_mV(slot%4,SAMPLE_COUNT); // read ac amplitude
	} else {
		amp_volt = readADC_SingleEnded(slot%4); // todo
	}
		

	logger.print(TOPIC_MQTT_PUBLISH, F(""));
	sprintf(m_msg_buffer,"Ads1115 Channel %i voltage: ",(slot%4));
	logger.p(m_msg_buffer);
	dtostrf(amp_volt, 3, 2, m_msg_buffer);
	logger.pln(m_msg_buffer);
	if(slot%4==0){
		return network.publish(build_topic(MQTT_ADS1015_A0_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else if(slot%4==1){
		return network.publish(build_topic(MQTT_ADS1015_A1_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else if(slot%4==2){
		return network.publish(build_topic(MQTT_ADS1015_A2_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else if(slot%4==3){
		return network.publish(build_topic(MQTT_ADS1015_A3_TOPIC,UNIT_TO_PC), m_msg_buffer);
	};
	return false;
}

// override-methode, only implement when needed
// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
//bool ads1015::subscribe(){
//	network.subscribe(build_topic(MQTT_ads1015_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
//	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_ads1015_TOPIC,PC_TO_UNIT), COLOR_GREEN);
//	return true;
//}

// override-methode, only implement when needed
// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
//bool ads1015::receive(uint8_t* p_topic, uint8_t* p_payload){
//	if (!strcmp((const char *) p_topic, build_topic(MQTT_ads1015_TOPIC,PC_TO_UNIT))) { // on / off with dimming
//		logger.println(TOPIC_MQTT, F("received ads1015 command"),COLOR_PURPLE);
//		// do something
//		return true;
//	}
//	return false; // not for me
//}

// override-methode, only implement when needed
// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool ads1015::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			for(uint8_t i=0; i<4; i++){
				char* t = discovery_topic_bake(MQTT_DISCOVERY_ADS_CHi_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
				char* m = new char[strlen(MQTT_DISCOVERY_ADS_CHi_MSG)+2*strlen(mqtt.dev_short)];
				sprintf(m, MQTT_DISCOVERY_ADS_CHi_MSG, mqtt.dev_short, i, mqtt.dev_short,i);
				logger.print(TOPIC_MQTT_PUBLISH, F("ADS discovery CH"), COLOR_GREEN);
				logger.pln(i);
				m_discovery_pub = network.publish(t,m);
				delete[] m;
				delete[] t;
			}
			return true;
		}
	}
#endif
	return false;
}

// Writes 16-bits to the specified destination register
void ads1015::writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
  Wire.beginTransmission(i2cAddress);
  Wire.write((uint8_t)reg);
  Wire.write((uint8_t)(value >> 8));
  Wire.write((uint8_t)(value & 0xFF));
  Wire.endTransmission();
}

// reads an 16 bit value from a given register
uint16_t ads1015::readRegister(uint8_t i2cAddress, uint8_t reg) {
  Wire.beginTransmission(i2cAddress);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  Wire.requestFrom(i2cAddress, (uint8_t)2);
  return ((Wire.read() << 8) | Wire.read());
}

// set gain, will be used in the configuration routines
void ads1015::setGain(uint16_t gain){
	m_gain = gain; 
}

// get gain, will be used in the configuration routines
uint16_t ads1015::getGain() { 
	return m_gain; 
}

// read a given amount of the samples and calc the max, min, avg and thus amplitude
float ads1015::readADC_continues_ac_mV(uint8_t channel, uint16_t sample) {
	float max_volt;
	float min_volt;
	float avg_volt;
	float amp_volt;
	int16_t max = -32767; // min signed int
	int16_t min = 32767; // Max signed int
	int32_t avg = 0; // sum, so start with 0
	int16_t res[sample];
	for(uint16_t i=0;i<sample;i++){
		while(!digitalRead(ADS_DATA_RDY_PIN)){ 
			yield();
		};
		res[i] = getLastConversionResults();
	}
	for(uint16_t i=0;i<sample;i++){
		if(res[i]>max){
			max=res[i];
		}
		if(res[i]<min){
			min=res[i];
		}
		avg+=res[i];
		//Serial.printf("%i;",res[i]);
	}
	// volt = zahl/65535*4.096*2 = zahl / 8 = zahl * 0,125
	max_volt = ((float)max)*0.125;
	min_volt = ((float)min)*0.125;
	avg_volt = (((float)avg)*0.125)/sample;
	amp_volt = max_volt-min_volt;

	// Debug
	Serial.print("Max ");
	Serial.print(max_volt);
	Serial.print(" min ");
	Serial.print(min_volt);
	Serial.print(" avg ");
	Serial.println(avg_volt);

	return amp_volt;
}

// configure ADC to confinues mode
bool ads1015::configureADC_continues(uint8_t channel) {
	if (channel > 3) {
		return false;
	}

	// Start with default values
	uint16_t config =
		ADS1015_REG_CONFIG_CQUE_4CONV |    // Disable the comparator (default val)
		ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
		ADS1015_REG_CONFIG_CPOL_ACTVHI | // Alert/Rdy active low   (default val)
		ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
		ADS1015_REG_CONFIG_DR_3300SPS |   // 3300 samples per second (default) 00c0 = 0000 0000 1110 0000
		ADS1015_REG_CONFIG_MODE_CONTIN;   // Continues mode (default)

	// Set PGA/voltage range
	config |= m_gain; 

	// Set single-ended input channel
	switch (channel) {
		case (0):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
			break;
		case (1):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
			break;
		case (2):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
			break;
		case (3):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
			break;
	}

	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;

	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_HITHRESH, 0x8123); // set to high ti FFFF and low to 0000 to use rdy line
	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_LOWTHRESH, 0x0000);
	
	// Write config register to the ADC
	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

	// give time to change
	delay(m_conversionDelay);
	
	// read last result to avoid reading old data, e.g. from last channel
	getLastConversionResults();

	return true;
}

// configure the ADC and read single shot wise 
uint16_t ads1015::readADC_SingleEnded(uint8_t channel) {
	if (channel > 3) {
		return 0;
	}

	// Start with default values
	uint16_t config =
		ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
		ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
		ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
		ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
		ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
		ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

	// Set PGA/voltage range
	config |= m_gain; 

	// Set single-ended input channel
	switch (channel) {
		case (0):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
			break;
		case (1):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
			break;
		case (2):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
			break;
		case (3):
			config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
			break;
	}

	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;

	// Write config register to the ADC
	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	delay(m_conversionDelay);

	// Read the conversion results
	// Shift 12-bit results right 4 bits for the ADS1015
	return readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT);
}

/**************************************************************************/
/*!
@brief  Reads the conversion results, measuring the voltage
difference between the P (AIN0) and N (AIN1) input.  Generates
a signed value since the difference can be either
positive or negative.
@return the ADC reading
*/
/**************************************************************************/
int16_t ads1015::readADC_Differential_0_1() {
	// Start with default values
	uint16_t config =
		ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
		ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
		ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
		ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
		ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
		ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

	// Set PGA/voltage range
	config |= m_gain;

	// Set channels
	config |= ADS1015_REG_CONFIG_MUX_DIFF_0_1; // AIN0 = P, AIN1 = N

	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;

	// Write config register to the ADC
	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	delay(m_conversionDelay);
	
	return getLastConversionResults();
}

/**************************************************************************/
/*!
@brief  Reads the conversion results, measuring the voltage
difference between the P (AIN2) and N (AIN3) input.  Generates
a signed value since the difference can be either
positive or negative.
@return the ADC reading
*/
/**************************************************************************/
int16_t ads1015::readADC_Differential_2_3() {
	// Start with default values
	uint16_t config =
		ADS1015_REG_CONFIG_CQUE_NONE |    // Disable the comparator (default val)
		ADS1015_REG_CONFIG_CLAT_NONLAT |  // Non-latching (default val)
		ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
		ADS1015_REG_CONFIG_CMODE_TRAD |   // Traditional comparator (default val)
		ADS1015_REG_CONFIG_DR_1600SPS |   // 1600 samples per second (default)
		ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

	// Set PGA/voltage range
	config |= m_gain;

	// Set channels
	config |= ADS1015_REG_CONFIG_MUX_DIFF_2_3; // AIN2 = P, AIN3 = N

	// Set 'start single-conversion' bit
	config |= ADS1015_REG_CONFIG_OS_SINGLE;

	// Write config register to the ADC
	writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

	// Wait for the conversion to complete
	delay(m_conversionDelay);
	
	return getLastConversionResults();
}

// read the conversion register
int16_t ads1015::getLastConversionResults() {
	// Read the conversion results
	return readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT);
}