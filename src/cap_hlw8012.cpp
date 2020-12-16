#include <cap_hlw8012.h>
#ifdef WITH_HLW
J_hlw8012::J_hlw8012(){};

J_hlw8012::~J_hlw8012(){
	logger.println(TOPIC_GENERIC_INFO, F("J_hlw8012 deleted"), COLOR_YELLOW);
};

uint8_t* J_hlw8012::get_key(){
	return (uint8_t*)"HLW";
}

void ICACHE_RAM_ATTR J_hlw8012_cf1_interrupt() {
	if(p_hlw){
		((J_hlw8012*)p_hlw)->m_hlw8012.cf1_interrupt();
	}
}

void ICACHE_RAM_ATTR J_hlw8012_cf_interrupt() {
	if(p_hlw){
		((J_hlw8012*)p_hlw)->m_hlw8012.cf_interrupt();
	}
}

bool J_hlw8012::init(){
	m_hlw8012 = HLW8012();
	m_hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);
	m_hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
	attachInterrupt(CF1_PIN, J_hlw8012_cf1_interrupt, CHANGE);
  attachInterrupt(CF_PIN, J_hlw8012_cf_interrupt, CHANGE);
	logger.println(TOPIC_GENERIC_INFO, F("HLW init"), COLOR_GREEN);
	return true;
}

uint8_t J_hlw8012::count_intervall_update(){
	return 3; // we have 1 values; temp that we want to publish per minute
}

// bool J_hlw8012::loop(){
// 	return false; // i did nothing
// }

bool J_hlw8012::intervall_update(uint8_t slot){
	if(slot == 0){
		sprintf(m_msg_buffer,"%i",m_hlw8012.getActivePower()); // integer
		logger.print(TOPIC_MQTT_PUBLISH, F("HLW power "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_HLW_POWER_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else if(slot == 1){
		dtostrf(m_hlw8012.getCurrent(), 2, 3, m_msg_buffer); // xx.xxx mA, double
		logger.print(TOPIC_MQTT_PUBLISH, F("HLW current "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_HLW_CURRENT_TOPIC,UNIT_TO_PC), m_msg_buffer);
	} else {
		sprintf(m_msg_buffer,"%i",m_hlw8012.getVoltage()); // integer
		logger.print(TOPIC_MQTT_PUBLISH, F("HLW voltage "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_HLW_VOLTAGE_TOPIC,UNIT_TO_PC), m_msg_buffer);
	}
		//Serial.printf("New mode %i\r\n",m_hlw8012.toggleMode());
	return false;
}

bool J_hlw8012::subscribe(){
	network.subscribe(build_topic(MQTT_HLW_CALIBRATE_TOPIC,PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_HLW_CALIBRATE_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

bool J_hlw8012::receive(uint8_t* p_topic, uint8_t* p_payload){
	if (!strcmp((const char *) p_topic, build_topic(MQTT_HLW_CALIBRATE_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received calibration command"),COLOR_PURPLE);

		// Let some time to register values
		unsigned long timeout = millis();
		while ((millis() - timeout) < 10000) {
		  delay(1);
		}

		// Calibrate using a 60W bulb (pure resistive) on a 230V line
		m_hlw8012.expectedActivePower(60.0);
		m_hlw8012.expectedVoltage(230.0);
		m_hlw8012.expectedCurrent(60.0 / 230.0);

		// Show corrected factors
		logger.p(F("[HLW] New current multiplier : "));
		logger.pln(m_hlw8012.getCurrentMultiplier());
		logger.p(F("[HLW] New voltage multiplier : "));
		logger.pln(m_hlw8012.getVoltageMultiplier());
		logger.p(F("[HLW] New power multiplier   : "));
		logger.pln(m_hlw8012.getPowerMultiplier());

		return true;
	}
	return false; // not for me
}

bool J_hlw8012::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
// bool J_hlw8012::publish(){
// 	return false;
// }
#endif
