#include <cap_GPIO.h>
#ifdef WITH_GPIO
// R,PI2,PW3,M3
// REMEMBER: we're talking about GPIO5 and NOT Arduino D5!!

// setup:
//   use capability GIN5 / GIP5 to configure GPIO5 as input (low active / high active)
//   use capability GOP5 / GON5 to configure GPIO5 as output (low active / high active) with half logarithmic
//   use capability GOLP5 / GOLN5 to configure GPIO5 as output (low active / high active) with linear dimming option

// operation:
//   input pins will report the logic level .. so a active low switch will send "ON" once the pin goes LOW (0V)
//   output pins can drive the pin ON / OFF / via PWM
//   simplest way -> Publish "ON"/"OFF" to topic "devXX/s/gpio_5_state" will set the pin to logic ON/OFF
//                   Response will be "ON/OFF" on "devXX/r/gpio_5_state"

//   PWM          -> Publish "27" to topic "devXX/s/gpio_5_state" will set the pin to logic 27 PWM .. mode depending
//   without         Response will be "ON" on "devXX/r/gpio_5_state"
//   dimming         and in addition "27" on "devXX/r/gpio_5_brightness"

//   dimm pin on  -> Publish "ON"/"OFF" to topic "devXX/s/gpio_5_dimm" will dimm the pin to logic ON/OFF
//   or off          Response will be "ON/OFF" on "devXX/r/gpio_5_state"
//                   and in addition "brightness" on "devXX/r/gpio_5_brightness" if the new state was ON

//   toggle       -> Publish any message to "devXX/s/gpio_5_toggle" to toggle the pin state
//                   Response will be "ON/OFF" on "devXX/r/gpio_5_state"
//                   and in addition "brightness" on "devXX/r/gpio_5_brightness" if the new state was ON

//   pluse        -> Publish "200" to "devXX/s/gpio_5_pulse" to set the pin ON for 200ms
//                   Response will be "ON" on "devXX/r/gpio_5_state"
//                   followed by "OFF" on "devXX/r/gpio_5_state" 200ms delayed



// simply the constructor
J_GPIO::J_GPIO(){
	for (uint8_t i = 0; i <= 16; i++) {
		m_discovery_pub[i]=false;
	};
};

// simply the destructor
J_GPIO::~J_GPIO(){
	// unload the dimmer
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_dimmer[i]) {
			delete m_dimmer[i];
			m_dimmer[i] = NULL;
		}

#ifdef WITH_DISCOVERY
		if(m_discovery_pub[i] & (timer_connected_start>0)){
			if (m_pin_out[i]) {
				char* t = discovery_topic_bake(MQTT_DISCOVERY_GPO_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
				logger.print(TOPIC_MQTT_PUBLISH, F("Erasing GPIO config "), COLOR_YELLOW);
				logger.pln(t);
				network.publish(t,(char*)"");
				m_discovery_pub[i] = false;
				delete[] t;

				// try to delete retained messages
				sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
				network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) "");
				sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC, i);
				network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) "");
			}
			if (m_pin_in[i]) {
				char* t = discovery_topic_bake(MQTT_DISCOVERY_GPI_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
				logger.print(TOPIC_MQTT_PUBLISH, F("Erasing GPIO config "), COLOR_YELLOW);
				logger.pln(t);
				network.publish(t,(char*)"");

				// try to delete retained messages
				sprintf(m_msg_buffer, MQTT_J_GPIO_INPUT_HOLD_TOPIC, i);
				network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) "");

				m_discovery_pub[i] = false;
				delete[] t;
			}
		}
#endif

	};
	logger.println(TOPIC_GENERIC_INFO, F("J_GPIO deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool J_GPIO::parse(uint8_t * config){
	bool f = false;

	// output
	for (uint8_t i = 0; i <= 16; i++) {
		if(i>=6 && i<=11){
			// 6 to 11 should not be used
			continue;
		}
		// output non inverted, half log output pwm values 0..99 -> 0-100%
		sprintf_P(m_msg_buffer, PSTR("GOP%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = false;
			m_dimmer[i]  = new dimmer(i, false, PWM_LOG);
			m_brightness[i].set(m_dimmer[i]->get_max());
			m_state[i].set(PWM_OFF);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// output inverted, half log output pwm values 0..99 -> 0-100%
		sprintf_P(m_msg_buffer, PSTR("GON%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = true;
			m_dimmer[i]  = new dimmer(i, true, PWM_LOG);
			m_brightness[i].set(m_dimmer[i]->get_max());
			m_state[i].set(PWM_OFF);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// output non inverted, linear output pwm values 0..255 -> 0-100%
		sprintf_P(m_msg_buffer, PSTR("GOLP%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = false;
			m_dimmer[i]  = new dimmer(i, false, PWM_LIN);
			m_brightness[i].set(m_dimmer[i]->get_max());
			m_state[i].set(PWM_OFF);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// output inverted, half linear output pwm values 0..255 -> 0-100%
		sprintf_P(m_msg_buffer, PSTR("GOLN%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = true;
			m_dimmer[i]  = new dimmer(i, true, PWM_LIN);
			m_brightness[i].set(m_dimmer[i]->get_max());
			m_state[i].set(PWM_OFF);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// input non inverted
		sprintf_P(m_msg_buffer, PSTR("GIP%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_in[i] = true; // mark pin as "in use"
			m_invert[i] = false;
			m_state[i].set(0);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// input inverted
		sprintf_P(m_msg_buffer, PSTR("GIN%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_in[i] = true; // mark pin as "in use"
			m_invert[i] = true;
			m_state[i].set(0);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
	}
	// set correct pwm range
	if (f) {
		analogWriteRange(255);
	}
	return f;
} // parse

uint8_t * J_GPIO::get_key(){
	return (uint8_t*)"GPO";
}

// will be callen if the m_pin_out is part of the config
bool J_GPIO::init(){
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_pin_out[i]) {
			pinMode(i, OUTPUT);
			digitalWrite(i, m_invert[i]);
			if (m_invert[i]) {
				sprintf_P(m_msg_buffer, PSTR("J_GPIO active low output on pin %i"), i);
			} else {
				sprintf_P(m_msg_buffer, PSTR("J_GPIO active high output on pin %i"), i);
			}
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		}
		if (m_pin_in[i]) {
			if(m_invert[i]){ // inverted input gets pull-up
				pinMode(i,INPUT_PULLUP);
				sprintf_P(m_msg_buffer, PSTR("J_GPIO active low input on pin %i"), i);
			} else {
				pinMode(i, INPUT);
				sprintf_P(m_msg_buffer, PSTR("J_GPIO active high input on pin %i"), i);
			}
			logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_GREEN);
		}
	}
	return true;
}


// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool J_GPIO::loop(){
	bool ret = false;

	for (uint8_t i = 0; i <= 16; i++) {
		// output
		if (m_pin_out[i]) {
			// output mode: m_timing_parameter is set to a non-zero value e.g. to pulse the output
			if (millis() >= m_timing_parameter[i] && m_timing_parameter[i] != 0) {
				set_output(i, 0);
				m_timing_parameter[i] = 0;
				m_state[i].set(PWM_OFF);         // publish "OFF" as text
			}
			// if one or more gpios are dimming, set return to true, to run uninterrupted
			else if (m_dimmer[i]->loop()) {
				ret = true;
			}
		}
		// INPUT
		else if (m_pin_in[i]) {
			// if parameter is high: set state to 1 and increase once per second
			// publish will send this to the server
			// we reset this as soon as the pin goes low
			if (digitalRead(i) != m_invert[i]) { // in hold mode, or pin is active
				if (m_timing_parameter[i] == 0) {
					m_state[i].set(1); // for input: 1 == on
					m_timing_parameter[i] = millis(); // timestamp of first push
				} else {
					m_state[i].check_set(min(10, (uint8_t) ((millis() - m_timing_parameter[i]) / 1000) + 1)); // will count the hold seconds up to 10
				}
			}
			// reset timing paramter once the pin was released again
			else if (m_timing_parameter[i]) {
				m_timing_parameter[i] = 0;
				m_state[i].check_set(0);
			}
		}
	}
	return ret; // i did nothing
} // loop

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
// bool J_GPIO::intervall_update(uint8_t slot){
// 	return false;
// }

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool J_GPIO::subscribe(){
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_pin_out[i]) {
			// set J_GPIO state
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
			network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

			// change J_GPIO state
			sprintf(m_msg_buffer, MQTT_J_GPIO_TOGGLE_TOPIC, i);
			network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

			// pulse the output
			sprintf(m_msg_buffer, MQTT_J_GPIO_PULSE_TOPIC, i);
			network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

			// brightness
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC, i);
			network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

			// dimming
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_DIMM_TOPIC, i);
			network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
			logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);
		}
	}

	// MASS dimming
	sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_DIMM_TOPIC, 255);
	network.subscribe(build_topic(m_msg_buffer, PC_TO_UNIT));
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(m_msg_buffer, PC_TO_UNIT), COLOR_GREEN);

	return true;
} // subscribe

// drive pin, values are logical not physical  so if your GPIO is active low and you set
// the intensity to MAX .. it will physically output LOW. will directly SET no dimming
// CALLEN BY:
//     Loop() when a pulse command was executed
//     receive() when single pin command, or toggle or pulse was received
// INPUT: pin and logical value of the brightness (0..255)
void J_GPIO::set_output(uint8_t pin, uint8_t intens_level){
	if (pin > 16) {
		return;
	}
	intens_level = min((int)(m_dimmer[pin]->get_max()), (int) intens_level); // limit
	if(intens_level){
		m_state[pin].set(PWM_ON); // store to trigger publish
	} else {
		m_state[pin].set(PWM_OFF); // store to trigger publish
	}

	// sort of debug
	sprintf(m_msg_buffer, "Set J_GPIO %i: %i%% = %i", pin, intens_level, intens[intens_level]);
	logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_PURPLE);

	m_dimmer[pin]->dimm_to(intens_level, false); // set pwm with no dimming
};


// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool J_GPIO::receive(uint8_t * p_topic, uint8_t * p_payload){
	// //////////////// MASS DIMMING //////////////////
	// MASS dimm pins .. e.g.  "gpio_255_dimm" >> "ON,13,15"
	sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_DIMM_TOPIC, 255);
	if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
		// get state ON / OFF
		bool state = false;
		if (!strncmp((const char *) p_payload, STATE_ON, 2)) {
			state = true;
		}
		uint8_t pin = 0;
		for (uint8_t i = 0; p_payload[i]; i++) {
			if (p_payload[i] >= '0' && p_payload[i] <= '9') {
				pin = pin * 10 + (p_payload[i] - '0');
			}
			// check if execution is up
			if ((p_payload[i] == ',' || p_payload[i + 1] == 0x00) && pin != 0) {
				// execute if configured
				if (m_pin_out[pin]) {
					m_dimmer[pin]->set_state(state);
					m_brightness[pin].outdated(true); // force republish for HA
					if (state) {
						m_state[pin].set(PWM_ON); // publish "ON" as text
					} else {
						m_state[pin].set(PWM_OFF); // publish "OFF" as text
					}
				}
				// reset pin to read the next in
				pin = 0;
			}
		}
		return true;
	}
	////////////////// END MASS DIMMING //////////////////

	////////////////// SINGLE PIN HANDLING //////////////////
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_pin_out[i]) {

			///// set pin state direcly ON or OFF or value, but no dimming  ////////
			// e.g. "gpio_5_state" -> "ON" / "OFF" / "124"
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off WITHOUT dimming
				// change OUTPUT
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					set_output(i, m_brightness[i].get_value());  // ON, no dimming
					// set_output will set m_state -> publish new state
				} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
					set_output(i, 0);  // full OFF, no dimming
					// set_output will set m_state -> publish new state
				} else { // set pwm level direct
					// try to get BRIGHTNESS
					m_brightness[i].set(min((int)(m_dimmer[i]->get_max()), atoi((const char *) p_payload))); // limit, depending on mode
					set_output(i,m_brightness[i].get_value());
					m_state[i].set(PWM_ON); // store to trigger publish
				}
				return true;
			}

			//// set target Brightness value, this will also dimm to the given brightness if the state was on ////
			// e.g. "gpio_5_brightness" -> "124"
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) {
				// change brightness
				m_brightness[i].set(min((int)(m_dimmer[i]->get_max()), atoi((const char *) p_payload))); // limit
				m_dimmer[i]->set_brightness(m_brightness[i].get_value()); // with dimming (optionally, if already on)
				return true;
			}

			//// dimm on or off ////
			// e.g. "gpio_5_dimm" -> "ON" / "OFF"
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_DIMM_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				// change OUTPUT
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					m_dimmer[i]->set_state(true);
					m_brightness[i].outdated(true); // force republish for HA
					m_state[i].set(PWM_ON);         // publish "ON" as text
				} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
					m_dimmer[i]->set_state(false);
					m_state[i].set(PWM_OFF); // publish "OFF" as text
				}
				return true;
			}

			//// toggle pin output ////
			// e.g. "gpio_5_toggle" -> "[doesn't matter]"
			sprintf(m_msg_buffer, MQTT_J_GPIO_TOGGLE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off WITHOUT dimming
				// set_output will also set state which will trigger the publishing
				if (m_state[i].get_value() == PWM_OFF) {
					set_output(i,m_brightness[i].get_value()); // direct set brightness value, no dimming
					m_brightness[i].outdated(true);
				} else {
					set_output(i, 0);
				}
				return true;
			}

			//// pulse output for some ms ////
			// e.g. "gpio_5_pulse" -> "[doesn't matter]"
			sprintf(m_msg_buffer, MQTT_J_GPIO_PULSE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // no dimming
				set_output(i,m_brightness[i].get_value()); // direct set brightness value, no dimming
				// set_output will also set state which will trigger the publishing
				uint16_t pulse_length = atoi((char *) p_payload);
				m_timing_parameter[i] = millis() + pulse_length;
				// do something
				return true;
			}
		}
	}
	////////////////// END SINGLE PIN HANDLING //////////////////
	// not for me
	return false;
} // receive

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool J_GPIO::publish(){
	// publish STATE and BRIGHTNESS after they've been set
	bool ret = false;

	for (uint8_t i = 0; i <= 16; i++) {
		// publish state, both in and out
		if (m_pin_out[i] || m_pin_in[i]) {
			// if there is a change
			if (m_state[i].get_outdated()) {
				sprintf(m_msg_buffer, "J_GPIO %i state ", i);
				logger.print(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);

				// OUTPUT, "ON" / "OFF" / PWM values set
				if (m_pin_out[i]) {
					sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
					if (m_state[i].get_value() == PWM_ON) {
						logger.pln((char *) STATE_ON);
						ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) STATE_ON);
					} else if (m_state[i].get_value() == PWM_OFF) {
						logger.pln((char *) STATE_OFF);
						ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) STATE_OFF);
					}
				}
				// INPUT: "ON" / "OFF" and the HOLD topic
				else {
					if (m_state[i].get_value() == 1 || m_state[i].get_value() == 0) { // right after push / release
						sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
						if (m_state[i].get_value()) {
							logger.pln((char *) STATE_ON);
							ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) STATE_ON);
						} else {
							logger.pln((char *) STATE_OFF);
							ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), (char *) STATE_OFF);
						}
					} else { // publishes on hold topics
						logger.pln((char) (m_state[i].get_value()));
						// publish
						sprintf(m_msg_buffer, MQTT_J_GPIO_INPUT_HOLD_TOPIC, i);
						sprintf(m_msg_buffer + strlen(m_msg_buffer) + 1, "%i", m_state[i].get_value() - 1);
						ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), m_msg_buffer + strlen(m_msg_buffer) + 1);
					}
				}

				if (ret) {
					m_state[i].outdated(false);
				}
				return ret; // one at the time
			}
			// brightness publishing required?
			else if (m_brightness[i].get_outdated()) {
				sprintf(m_msg_buffer, "J_GPIO %i brightness ", i);
				logger.print(TOPIC_MQTT_PUBLISH, m_msg_buffer, COLOR_GREEN);

				char t[5];
				sprintf(t, "%i", m_brightness[i].get_value());
				logger.pln(t);
				sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC, i);
				ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), t);

				if (ret) {
					m_brightness[i].outdated(false);
				}
				return ret; // one at the time
			}
		}

#ifdef WITH_DISCOVERY
		if(!m_discovery_pub[i]){
			if(millis()-timer_connected_start>(uint32_t)(NETWORK_SUBSCRIPTION_DELAY+300*i)){
				if (m_pin_out[i]) {
					char* t = discovery_topic_bake(MQTT_DISCOVERY_GPO_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
					char* m = new char[strlen(MQTT_DISCOVERY_GPO_MSG)+3*strlen(mqtt.dev_short)];
					sprintf(m, MQTT_DISCOVERY_GPO_MSG, mqtt.dev_short, i, mqtt.dev_short, i, mqtt.dev_short, i);
					logger.println(TOPIC_MQTT_PUBLISH, F("GPIO discovery"), COLOR_GREEN);
					//logger.p(t);
					//logger.p(" -> ");
					//logger.pln(m);
					m_discovery_pub[i] = network.publish(t,m);
					delete[] m;
					delete[] t;
				}
				if (m_pin_in[i]) {
					char* t = discovery_topic_bake(MQTT_DISCOVERY_GPI_TOPIC,mqtt.dev_short,i); // don't forget to "delete[] t;" at the end of usage;
					char* m = new char[strlen(MQTT_DISCOVERY_GPI_MSG)+2*strlen(mqtt.dev_short)];
					sprintf(m, MQTT_DISCOVERY_GPI_MSG, mqtt.dev_short, i, mqtt.dev_short, i);
					logger.println(TOPIC_MQTT_PUBLISH, F("GPIO discovery"), COLOR_GREEN);
					//logger.p(t);
					//logger.p(" -> ");
					//logger.pln(m);
					m_discovery_pub[i] = network.publish(t,m);
					delete[] m;
					delete[] t;
				}
			}
		}
#endif
	} // loop
	return ret;
}  // publish

//  ---------------------------------------- ///
//  ---------------- DIMMER ---------------- ///
//  ---------------------------------------- ///
dimmer::dimmer(uint8_t gpio, bool invers, uint8_t phy_log){
	m_step_time = 10; // ms
	m_gpio      = gpio;
	m_invers    = invers;
	m_phy_log   = phy_log;
	m_end_time  = 0;
	if(m_phy_log == PWM_LIN){
		m_backup_v  = PWM_LIN_MAX; // override by MQTT msg
	} else {
		m_backup_v  = PWM_LOG_MAX; // override by MQTT msg
	}
};

// returns the maximum allowed input value
uint8_t dimmer::get_max(){
	if(m_phy_log == PWM_LIN){
		return PWM_LIN_MAX; // 255
	}
	return PWM_LOG_MAX; // 99
}

// actually executing the m_dimmer
// return true if we're dimming, will avoid mqtt msg during dimming
// return false if the dimmer is off
bool dimmer::loop(){
	if (m_end_time) { // we are dimming as long as this is non-zero
		// logger.p(".");
		if (millis() >= m_next_time) {
			// erial.printf("%i > %i\r\n",m_next_time,m_end_time);
			m_next_time = millis() + m_step_time; // save for next round
			// set new value
			if (m_next_time > m_end_time) {
				// erial.println("end time");
				m_current_v = m_target_v;
				m_end_time  = 0; // last step, stop dimming
			} else {
				m_current_v = min((int)get_max(), (int) map(m_next_time - m_step_time, m_start_time, m_end_time, m_start_v, m_target_v));
			}
			// logger.pln(m_light_current.r);``
			uint8_t analog_value = m_current_v; // assume linear and non invers
			if(m_phy_log == PWM_LIN){
				if(m_invers){
					analog_value = get_max() - m_current_v; // linear but invers
				}
			} else {
				if(m_invers) {
					analog_value=intens[get_max() - m_current_v]; // log inverse
				} else {
					analog_value=intens[m_current_v]; // log non invers
				}
			}
			// erial.printf("%i,%i\r\n",m_gpio,analog_value);
			analogWrite(m_gpio, analog_value); // finally write to pin
		}
		return true; // muy importante .. request uninterrupted execution
	}
	return false;
};

// if the light is on: dimm to new value, else set the backup value, so we're going
// to dimm to this as soon as we turn the pin "ON"
bool dimmer::set_brightness(uint8_t t){
	return set_brightness(t, true);
}

bool dimmer::set_brightness(uint8_t t, bool dimming){
	t = min((int) t, (int)get_max());
	// if the light is already on, dimm to new brightness
	if (m_current_v) {
		dimm_to(t, dimming);
	} else { // else prepare to dimm on to new value
		m_backup_v = t;
	}
	return true;
};

// prepare the paramter for the dimmer, setting m_end_time will trigger the dimmer
void dimmer::dimm_to(uint8_t t){
	return dimm_to(t, true);
}

void dimmer::dimm_to(uint8_t t, bool dimming){
	if (dimming) {               // std: with dimming
		m_start_v    = m_current_v; // used current values as new start value
		m_target_v   = t;
		m_start_time = millis();
		m_end_time   = m_start_time + abs(m_current_v - m_target_v) * m_step_time;
		m_next_time  = m_start_time + m_step_time;
		// erial.printf("set end time to %i\r\n",m_end_time);
	} else {          // this is used if we only want to update our internal states
		m_target_v  = t; // the dimm to value
		m_end_time  = 1; // not 0 but very low to trigger loop
		m_next_time = 2; // next > end to directly jump to the end state
		loop();          // run, will set the t value instantly
	}
};

// call brightess 3, call dimm ON
// dimm ON -> set_state(true) -> dimm_to(m_backup_v)

// switch the dimmer ON or OFF, basically dimm to 0 or back to the backup value
void dimmer::set_state(bool s){
	if (s) { // turn ON
		// erial.printf("ON %i\r\n",m_gpio);
		// DIMM ON (to backup) if we're not dimming at this very point
		if (m_end_time == 0) {
			dimm_to(m_backup_v);
		}
	} else { // turn OFF
		// avoid override backup with target when target is 0, e.g. when we receive OFF twice
		if (m_target_v != 0) {
			m_backup_v = m_target_v;
		}
		dimm_to(0);
	}
};
//  ---------------------------------------- ///
//  ---------------- DIMMER ---------------- ///
//  ---------------------------------------- ///
#endif
