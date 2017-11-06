#include <light.h>

light::light(){
	m_light_target  = { 99, 99, 99 };
	m_light_start   = { 99, 99, 99 };
	m_light_current = { 99, 99, 99 }; // actual value written to PWM
	m_light_backup  = { 99, 99, 99 }; // to be able to resume "dimm on" to the last set color
	m_pwm_dimm_time = 10;             // 10ms per Step, 255*0.01 = 2.5 sec
};
light::~light(){
	logger.println(TOPIC_GENERIC_INFO, F("light deleted"), COLOR_YELLOW);
};

uint8_t * light::get_key(){
	sprintf((char *) key, "LIG");
	return key;
}

bool light::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool light::init(){
	timer_dimmer       = 0;
	timer_dimmer_start = 0;
	timer_dimmer_end   = 0;
	type = 0;
	logger.println(TOPIC_GENERIC_INFO, F("light init"), COLOR_GREEN);
}

void light::interrupt(){};
uint8_t light::count_intervall_update(){	return 0;	};

bool light::reg_provider(peripheral * p, uint8_t t){
	type     = t;
	provider = p;
	logger.print(TOPIC_GENERIC_INFO, F("light registered provider: "), COLOR_GREEN);
	if(t==T_SL){
		Serial.println(F("simple light"));
	} else if(t==T_PWM){
		Serial.println(F("PWM light"));
	} else if(t==T_NEO){
		Serial.println(F("neostrip light"));
	} else if(t==T_BOne){
		Serial.println(F("B1 light"));
	} else if(t==T_AI){
		Serial.println(F("AI light"));
	} else {
		Serial.println(F("UNKNOWN "));
		Serial.println(t);
	}

}

void light::setState(bool state){
	if (m_animation_type.get_value()) { // if there is an animation, switch it off
		setAnimationType(ANIMATION_OFF);
	}
	if(state){
		if (m_state.get_value() != true) {
			m_state.set(true);
			// bei diesem topic hartes einschalten
			m_light_current = m_light_backup;
			send_current_light();
			m_light_brightness.outdated();
		}
	} else {
		if (m_state.get_value() != false) {
			m_state.set(false);
			m_light_backup  = m_light_target; // save last target value to resume later on
			m_light_current = (led){ 0, 0, 0 };
			send_current_light();
		}
	}
}

void light::setColor(uint8_t r, uint8_t g, uint8_t b){
	uint32_t c=(uint32_t)r<<8+(uint32_t)g<<4+b;
	if(m_state.get_value()){
		m_light_color.set(c);
		m_light_current.r = r;
		m_light_current.g = g;
		m_light_current.b = b;
		send_current_light();
	} else {
		m_light_backup.r = r;
		m_light_backup.g = g;
		m_light_backup.b = b;
	}
	return;
}

void light::toggle(){
	if (!m_state.get_value()) {
		m_light_current = m_light_backup;
	} else {
		m_light_backup  = m_light_target; // save last target value to resume later on
		m_light_current = (led){ 0, 0, 0 };
	}
	send_current_light();
	m_state.set(!m_state.get_value());
}


bool light::loop(){
	// // dimming active?  ////
	if (timer_dimmer_end) { // we are dimming as long as this is non-zero
		Serial.print(".");
		if (millis() >= timer_dimmer + m_pwm_dimm_time) {
			// Serial.print("DIMMER ");
			timer_dimmer = millis(); // save for next round

			// set new value
			if (timer_dimmer + m_pwm_dimm_time > timer_dimmer_end) {
				m_light_current  = m_light_target;
				timer_dimmer_end = 0; // last step, stop dimming
			} else {
				m_light_current.r = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.r, m_light_target.r);
				m_light_current.g = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.g, m_light_target.g);
				m_light_current.b = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.b, m_light_target.b);
				//m_light_current.r = intens[_min(m_light_current.r,sizeof(intens)-1)];
				//m_light_current.g = intens[_min(m_light_current.g,sizeof(intens)-1)];
				//m_light_current.b = intens[_min(m_light_current.b,sizeof(intens)-1)];
			}
			Serial.println(m_light_target.r);
			send_current_light();
		}
		return true; // muy importante .. request uninterrupted execution
	}

	// // RGB circle ////
	if (m_animation_type.get_value()) {
		if (millis() >= m_animation_dimm_time) {
			if (type == T_NEO) {
				if (m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL) {
					for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
						RgbColor c = Wheel(((i * 256 / NEOPIXEL_LED_COUNT) + m_animation_pos) & 255);
						((NeoStrip *) provider)->setPixelColor(c.R, c.G, c.B, i);
					}
					m_animation_pos++; // move wheel by one, will overrun and therefore cycle
					((NeoStrip *) provider)->show();
					m_animation_dimm_time = millis() + ANIMATION_STEP_TIME; // schedule update
				} else if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
					RgbColor c = Wheel(m_animation_pos & 255);
					for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
						((NeoStrip *) provider)->setPixelColor(c.R, c.G, c.B, i);
					}
					m_animation_pos++; // move wheel by one, will overrun and therefore cycle
					((NeoStrip *) provider)->show();
					m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME; // schedule update
				} else if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
					// m_animation_pos geht bis 255/25 pixel = 10 colors,das wheel hat 255 pos, also 25 per 10 .. heh?
					RgbColor c = Wheel(int(m_animation_pos / NEOPIXEL_LED_COUNT) * 76 & 255);
					((NeoStrip *) provider)->setPixelColor(c.R, c.G, c.B, m_animation_pos % NEOPIXEL_LED_COUNT);
					// max: 255/10=25,25*10 to scale, 250*3 *3 will jump to various colors
					m_animation_pos++; // move wheel by one, will overrun and therefore cycle
					((NeoStrip *) provider)->show();
					m_animation_dimm_time = millis() + 3 * ANIMATION_STEP_TIME; // schedule update
				}
			} else if (type == T_PWM || type == T_BOne || type == T_AI) {
				if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
					RgbColor w = Wheel(m_animation_pos & 255);
					if(type == T_PWM){
						((PWM *) provider)->setColor(w.R, w.G, w.B);                          // last two: warm white, cold white
					} else if (type == T_BOne) {
						((BOne *) provider)->setColor(w.R, w.G, w.B);                          // last two: warm white, cold white
					} else if (type == T_AI) {
						((AI *) provider)->setColor(w.R, w.G,
								  w.B);                          // last two: warm white, cold white
					}
					m_animation_pos++;                                          // move wheel by one, will overrun and therefore cycle
					m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME; // schedule update
				}
			}
		} // timer
	}  // if active
	   // // RGB circle ////
	return false; // i did nothing
} // loop

bool light::intervall_update(uint8_t slot){  return false; }

bool light::subscribe(){
	client.subscribe(build_topic(MQTT_LIGHT_COMMAND_TOPIC)); // on off
	client.loop();
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_COMMAND_TOPIC), COLOR_GREEN);

	if (type > T_SL) { // pwm, neo, b1, AI
		////////////////// color topic, ////////////////////////////////
		//subscribe this before brightness topic
		client.subscribe(build_topic(MQTT_LIGHT_COLOR_COMMAND_TOPIC));
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_COLOR_COMMAND_TOPIC), COLOR_GREEN);

		client.subscribe(build_topic(MQTT_LIGHT_DIMM_COLOR_COMMAND_TOPIC));
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_COLOR_COMMAND_TOPIC), COLOR_GREEN);

		////////////////// brightness topic, ////////////////////////////////
		//subscribe this before the on off!
		client.subscribe(build_topic(MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC));
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC), COLOR_GREEN);

		client.subscribe(build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_COMMAND_TOPIC));
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_COMMAND_TOPIC), COLOR_GREEN);

		////////////////// dimm topic, ////////////////////////////////
		// on /off topics
		client.subscribe(build_topic(MQTT_LIGHT_DIMM_COMMAND_TOPIC)); // dimm on
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_COMMAND_TOPIC), COLOR_GREEN);

		client.subscribe(build_topic(MQTT_LIGHT_DIMM_DELAY_COMMAND_TOPIC));
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_DELAY_COMMAND_TOPIC), COLOR_GREEN);
	}
	if (type > T_PWM) { // neo, b1, ai
		client.subscribe(build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_COMMAND_TOPIC)); // simple rainbow  topic
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_COMMAND_TOPIC), COLOR_GREEN);
	}

	if (type == T_NEO) {
		client.subscribe(build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_COMMAND_TOPIC)); // rainbow  topic
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_COMMAND_TOPIC), COLOR_GREEN);

		client.subscribe(build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_COMMAND_TOPIC)); // color WIPE topic
		client.loop();
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_COMMAND_TOPIC), COLOR_GREEN);
	}

	return true;
} // subscribe

bool light::receive(uint8_t * p_topic, uint8_t * p_payload){
	// dimm to given PWM value
	if ( (!strcmp((const char *) p_topic,
	    build_topic(MQTT_LIGHT_COMMAND_TOPIC))) ||
	  (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_COMMAND_TOPIC)))  ) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON)) {
			if (m_state.get_value() != true) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) { // if there is an animation, switch it off
					setAnimationType(ANIMATION_OFF);
				}

				m_state.set(true);
				// bei diesem topic hartes einschalten
				m_light_current = m_light_backup;
				send_current_light();
				// Home Assistant will assume that the pwm light is 100%, once we "turn it on"
				// but it should return to whatever the m_light_brithness is
				m_light_brightness.outdated();
			} else {
				// was already on .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
			if (m_state.get_value() != false) {
				m_state.set(false);
				m_light_backup  = m_light_target; // save last target value to resume later on
				m_light_current = (led){ 0, 0, 0 };
				send_current_light();
			} else {
				// was already off .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		}
	}
	// dimm to given PWM value
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_COMMAND_TOPIC))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F(" received dimm command"));
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON)) {
			if (m_state.get_value() != true) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) {
					setAnimationType(ANIMATION_OFF);
				}
				// Serial.println("light was off");
				m_state.set(true);
				m_light_color.set(m_light_target.r + m_light_target.g + m_light_target.b); // set this to publish that we've left the color
				// bei diesem topic ein dimmen
				DimmTo(m_light_backup);
			} else {
				// was already on .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
			if (m_state.get_value() != false) {
				// Serial.println("light was on");
				m_state.set(false);
				m_light_color.set(m_light_target.r + m_light_target.g + m_light_target.b); // set this to publish that we've left the color
				// remember the current target value to resume to this value once we receive a 'dimm on 'command
				m_light_backup = m_light_target; // target instead off current (just in case we are currently dimming)
				DimmTo((led){ 0, 0, 0 });        // dimm off
			} else {
				// was already off .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		}
	}
	// //////////////////////// SET RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_RAINBOW_WHEEL) {
			setAnimationType(ANIMATION_RAINBOW_WHEEL); // triggers also the publishing
		} else if (!strcmp_P((const char *) p_payload,
		   STATE_OFF) && m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL)
		{
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			m_animation_type.outdated();
		}
	}
	// //////////////////////// SET RAINBOW /////////////////
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_RAINBOW_SIMPLE) {
			setAnimationType(ANIMATION_RAINBOW_SIMPLE);
		} else if (!strcmp_P((const char *) p_payload,
		   STATE_OFF) && m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE)
		{
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			m_animation_type.outdated();
		}
	}
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_COLOR_WIPE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF) && m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			m_animation_type.outdated();
		}
	}
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////
	// //////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_BRIGHTNESS_COMMAND_TOPIC))) { // directly set the color, hard
		m_light_current.r = atoi((const char *) p_payload);                                          // das regelt die helligkeit
		m_light_current.g = atoi((const char *) p_payload);                                          // das regelt die helligkeit
		m_light_current.b = atoi((const char *) p_payload);                                          // das regelt die helligkeit
		m_light_target = m_light_current;
		if (m_light_current.r) {
			m_state.set(true); // simply set, will trigger a publish
		} else {
			m_state.set(false); // simply set, will trigger a publish
		}
		send_current_light();
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_COMMAND_TOPIC))) { // smooth dimming of pwm
		uint8_t t = (uint8_t) atoi((const char *) p_payload);
		Serial.print(F("pwm dimm input "));
		Serial.println(t);
		if (t) {
			m_state.set(true); // simply set, will trigger a publish
		} else {
			m_state.set(false); // simply set, will trigger a publish
		}
		DimmTo((led){ t, t, t });
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_COLOR_COMMAND_TOPIC))) { // directly set rgb, hard
		Serial.println(F("set input hard"));

		uint8_t color[3] = { 0, 0, 0 };
		uint8_t s        = 0;
		for (uint8_t i = 0; p_payload[i]; i++) {
			if (p_payload[i] == ',') {
				s = (s + 1) % 3;
			} else {
				color[s] = color[s] * 10 + p_payload[i] - '0';
			}
		}

		m_light_current = (led){
			(uint8_t) map(color[0], 0, 255, 0, 99),
			(uint8_t) map(color[1], 0, 255, 0, 99),
			(uint8_t) map(color[2], 0, 255, 0, 99)
		};
		m_light_target = m_light_current;
		if (m_light_current.r + m_light_current.g + m_light_current.b > 0) {
			m_state.set(true); // simply set, will trigger a publish
		} else {
			m_state.set(false); // simply set, will trigger a publish
		}
		send_current_light();
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_COLOR_COMMAND_TOPIC))) { // smoothly dimm to rgb value
		Serial.println(F("color dimm input"));

		uint8_t color[3] = { 0, 0, 0 };
		uint8_t s        = 0;
		for (uint8_t i = 0; p_payload[i]; i++) {
			if (p_payload[i] == ',') {
				s = (s + 1) % 3;
			} else {
				color[s] = color[s] * 10 + p_payload[i] - '0';
			}
		}

		if (color[0] + color[1] + color[2] > 0) {
			m_state.set(true); // simply set, will trigger a publish
		} else {
			m_state.set(false); // simply set, will trigger a publish
		}
		// input color values are 888rgb .. DimmTo needs precentage which it will convert it into half log brighness
		DimmTo((led){
		   (uint8_t) map(color[0], 0, 255, 0, 99),
		   (uint8_t) map(color[1], 0, 255, 0, 99),
		   (uint8_t) map(color[2], 0, 255, 0, 99)
				});
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_DELAY_COMMAND_TOPIC))) { // adjust dimmer delay
		m_pwm_dimm_time = atoi((const char *) p_payload);
		// Serial.print("Setting dimm time to: ");
		// Serial.println(m_pwm_dimm_time);
	}
	return false; // not for me
} // receive

bool light::publish(){
	if (!publishLightState()) {
		if (!publishLightBrightness()) {
			if (!publishRGBColor()) {
				return false;
			}
		}
	}
	return true;
}

bool light::publishLightState(){
	if (m_state.get_outdated()) {
		boolean ret = false;

		logger.print(TOPIC_MQTT_PUBLISH, F("light state "), COLOR_GREEN);
		if (m_state.get_value()) {
			Serial.println(STATE_ON);
			ret = client.publish(build_topic(MQTT_LIGHT_STATE_TOPIC), STATE_ON, true);
		} else {
			Serial.println(STATE_OFF);
			ret = client.publish(build_topic(MQTT_LIGHT_STATE_TOPIC), STATE_OFF, true);
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}
	return false;
}

// function called to publish the brightness of the led
bool light::publishLightBrightness(){
	if (m_light_brightness.get_outdated()) {
		boolean ret = false;
		logger.print(TOPIC_MQTT_PUBLISH, F("light brightness "), COLOR_GREEN);
		Serial.println(m_light_target.r);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_light_target.r);
		ret = client.publish(build_topic(MQTT_LIGHT_BRIGHTNESS_STATE_TOPIC), m_msg_buffer, true);
		if (ret) {
			m_light_brightness.set(m_light_target.r, false); // required?
		}
		return ret;
	}
	return false;
}

// function called to publish the color of the led
bool light::publishRGBColor(){
	if (m_light_color.get_outdated()) {
		boolean ret = false;
		logger.print(TOPIC_MQTT_PUBLISH, F("light color "), COLOR_GREEN);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", m_light_target.r, m_light_target.g, m_light_target.b);
		Serial.println(m_msg_buffer);
		ret = client.publish(build_topic(MQTT_LIGHT_COLOR_STATE_TOPIC), m_msg_buffer, true);
		if (ret) {
			m_light_color.outdated(false);
		}
		return ret;
	}
	return false;
}

void light::DimmTo(led dimm_to){
	Serial.print(F("DimmTo: "));
	Serial.print(dimm_to.r);
	Serial.print(",");
	Serial.print(dimm_to.g);
	Serial.print(",");
	Serial.println(dimm_to.b);
	// find biggest difference for end time calucation
	m_light_target = dimm_to;

	uint8_t biggest_delta = 0;
	uint8_t d = abs(m_light_current.r - dimm_to.r);
	if (d > biggest_delta) {
		biggest_delta = d;
	}
	d = abs(m_light_current.g - dimm_to.g);
	if (d > biggest_delta) {
		biggest_delta = d;
	}
	d = abs(m_light_current.b - dimm_to.b);
	if (d > biggest_delta) {
		biggest_delta = d;
	}
	m_light_start = m_light_current;

	timer_dimmer_start = millis();
	timer_dimmer_end   = timer_dimmer_start + biggest_delta * m_pwm_dimm_time;

	// Serial.print("Enabled dimming, timing: ");
	// Serial.println(m_pwm_dimm_time);
}

// set rainbow start point .. thats pretty much it
void light::setAnimationType(int type){
	// if we're starting a animation and have been on pwm light, switch pwm off
	if (m_state.get_value() && type != ANIMATION_OFF) {
		m_state.set(false);                  // triggers update
		m_light_backup    = m_light_current; // store to resume state
		m_light_current.r = 0;               // this will lead to the correct state dimm wise
		m_light_current.g = 0;               //  if we leave them at whereever they are, switch to animation
		m_light_current.b = 0;               // and switch back  to dimm light no dimming would happen cause the sys thinks it still/already there
	}

	// animation processing
	m_animation_pos       = 0; // start all animation from beginning
	m_animation_dimm_time = millis();
	m_animation_type.set(type);
	// switch off
	if (m_animation_type.get_value() == ANIMATION_OFF) {
		if (type == T_PWM) {
			((PWM *) provider)->setColor(0, 0, 0);
		} else if (type == T_NEO) {
			((NeoStrip *) provider)->setColor(0, 0, 0);
		} else if (type == T_BOne) {
			((BOne *) provider)->setColor(0, 0, 0);
		} else if (type == T_AI) {
			((AI *) provider)->setColor(0, 0, 0);
		}
	}
}

void light::send_current_light(){
	if (type == T_SL) {
		((simple_light *) provider)->setColor(m_light_current.r, m_light_current.g, m_light_current.b);
	} else if (type == T_PWM) {
		((PWM *) provider)->setColor(m_light_current.r, m_light_current.g, m_light_current.b);
	} else if (type == T_NEO) {
		((NeoStrip *) provider)->setColor(m_light_current.r, m_light_current.g, m_light_current.b);
	} else if (type == T_BOne) {
		((BOne *) provider)->setColor(m_light_current.r, m_light_current.g, m_light_current.b);
	} else if (type == T_AI) {
		((AI *) provider)->setColor(m_light_current.r, m_light_current.g, m_light_current.b);
	}
}

RgbColor light::Wheel(byte WheelPos){
	WheelPos = 255 - WheelPos;
	if (WheelPos < 85) {
		return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
	}
	WheelPos -= 170;
	return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}
