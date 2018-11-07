#include <cap_GPIO.h>
#ifdef WITH_GPIO
// R,PI2,PW3,M3
// REMEMBER: we're talking about GPIO5 and NOT Arduino D5!!
// simply the constructor
J_GPIO::J_GPIO(){ };

// simply the destructor
J_GPIO::~J_GPIO(){
	// unload the dimmer
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_dimmer[i]) {
			delete m_dimmer[i];
			m_dimmer[i] = NULL;
		}
	}
	;
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
		// output non inverted
		sprintf_P(m_msg_buffer, PSTR("GOP%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = false;
			m_dimmer[i]  = new dimmer(i, false);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// output inverted
		sprintf_P(m_msg_buffer, PSTR("GON%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_out[i] = true; // mark pin as "in use"
			m_invert[i]  = true;
			m_dimmer[i]  = new dimmer(i, true);
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// input non inverted
		sprintf_P(m_msg_buffer, PSTR("GIP%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_in[i] = true; // mark pin as "in use"
			m_invert[i] = false;
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
		// input inverted
		sprintf_P(m_msg_buffer, PSTR("GIN%i"), i);
		if (cap.parse(config, (uint8_t *) m_msg_buffer)) {
			m_pin_in[i] = true; // mark pin as "in use"
			m_invert[i] = true;
			f = true; // at least one pin  used, so keep this component alive
			continue;
		}
	}
	// set correct pwm range
	if (f) {
#ifndef ESP32
		analogWriteRange(255);
#endif
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
			pinMode(i, INPUT);
			digitalWrite(i, m_invert[i]); // inverted input gets pull-up
			if (m_invert[i]) {
				sprintf_P(m_msg_buffer, PSTR("J_GPIO active low input on pin %i"), i);
			} else {
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
			if (millis() >= m_timing_parameter[i] && m_timing_parameter[i] != 0) {
				set_output(i, 0);
				m_timing_parameter[i] = 0;
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
					m_state[i].set(1); // 1 == on
					m_timing_parameter[i] = millis(); // timestamp of first push
				} else {
					m_state[i].check_set(_min(10, (uint8_t) ((millis() - m_timing_parameter[i]) / 1000) + 1)); // will count the hold seconds up to 10
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

// drive pin
void J_GPIO::set_output(uint8_t pin, uint8_t intens_level){
	if (pin > 16) {
		return;
	}
	if (intens_level == PWM_ON || intens_level == PWM_OFF) { // non - pwm / ON-OFF
		m_state[pin].set(intens_level);                         // store to trigger publish

		if (m_state[pin].get_value() == PWM_ON) {
			sprintf(m_msg_buffer, "Set J_GPIO %i: ON", pin);
			m_dimmer[pin]->dimm_to(PWM_MAX, false); // max on with no dimming
		} else {
			sprintf(m_msg_buffer, "Set J_GPIO %i: OFF", pin);
			m_dimmer[pin]->dimm_to(0, false); // off with no dimming;
		}
		logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_PURPLE);
	} else {                                          // pwm
		intens_level = _min(PWM_MAX, (int) intens_level); // limit
		m_state[pin].set(intens_level);                  // store to trigger publish

		// sort of debug
		sprintf(m_msg_buffer, "Set J_GPIO %i: %i%% = %i", pin, intens_level, intens[intens_level]);
		logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_PURPLE);

		m_dimmer[pin]->dimm_to(m_state[pin].get_value(), false); // set pwm with no dimming
	}
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
	// //////////////// MASS DIMMING //////////////////

	// single pin dimming, not 100% in syn
	for (uint8_t i = 0; i <= 16; i++) {
		if (m_pin_out[i]) {
			// set pin once and direcly ON or OFF
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_STATE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				// change OUTPUT
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					set_output(i, PWM_ON); // 99 == 100% (0..99)
				} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
					set_output(i, PWM_OFF); // 0%
				} else {                 // set pwm level direct
					// try to get BRIGHTNESS
					m_brightness[i].set(_min(PWM_MAX, atoi((const char *) p_payload)));
					set_output(i, _min(PWM_MAX, atoi((const char *) p_payload))); // limit to 0..99
				}
				return true;
			}

			// set Brightness value, this will also dimm to the given brightness if the state was on
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_BRIGHTNESS_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				// change brightness
				m_brightness[i].set(_min(PWM_MAX, atoi((const char *) p_payload)));
				m_dimmer[i]->set_brightness(m_brightness[i].get_value());
				return true;
			}

			// dimm on or off
			sprintf(m_msg_buffer, MQTT_J_GPIO_OUTPUT_DIMM_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				// change OUTPUT
				if (!strcmp_P((const char *) p_payload, STATE_ON)) {
					m_dimmer[i]->set_state(true);
					m_brightness[i].outdated(true); // force republish for HA
					m_state[i].set(PWM_ON);         // publish "OFF" as text
				} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
					m_dimmer[i]->set_state(false);
					m_state[i].set(PWM_OFF); // publish "OFF" as text
				}
				return true;
			}

			// toggle pin output
			sprintf(m_msg_buffer, MQTT_J_GPIO_TOGGLE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				if (m_state[i].get_value() == PWM_ON) {
					set_output(i, PWM_OFF);
				} else {
					set_output(i, PWM_ON);
				}
				return true;
			}

			// pulse output for some ms
			sprintf(m_msg_buffer, MQTT_J_GPIO_PULSE_TOPIC, i);
			if (!strcmp((const char *) p_topic, build_topic(m_msg_buffer, PC_TO_UNIT))) { // on / off with dimming
				set_output(i, PWM_ON);
				uint16_t pulse_length = atoi((char *) p_payload);
				m_timing_parameter[i] = millis() + pulse_length;
				// do something
				return true;
			}
		}
	}
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
					} else { // pwm
						char t[5];
						sprintf(t, "%i", m_state[i].get_value());
						logger.pln(t);
						ret = network.publish(build_topic(m_msg_buffer, UNIT_TO_PC), t);
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
	} // loop
	return ret;
}  // publish

//  ---------------------------------------- ///
//  ---------------- DIMMER ---------------- ///
//  ---------------------------------------- ///
dimmer::dimmer(uint8_t gpio, bool invers){
	m_step_time = 10; // ms
	m_gpio      = gpio;
	m_invers    = invers;
	m_backup_v  = PWM_MAX; // override by MQTT msg
};

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
				m_current_v = _min(PWM_MAX, (int) map(m_next_time - m_step_time, m_start_time, m_end_time, m_start_v, m_target_v));
			}
			// logger.pln(m_light_current.r);``
#ifndef ESP32
			if (m_invers) {
				// erial.printf("%i,%i\r\n",m_gpio,intens[99 - m_current_v]);
				analogWrite(m_gpio, intens[PWM_MAX - m_current_v]);
			} else {
				// erial.printf("%i,%i\r\n",m_gpio,intens[m_current_v]);
				analogWrite(m_gpio, intens[m_current_v]);
			}
#endif
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
	t = _min((int) t, PWM_MAX);
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
		m_next_time = 2; // next > end to directly ump to the end state
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
