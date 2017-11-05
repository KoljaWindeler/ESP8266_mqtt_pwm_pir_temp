#include <animation.h>

animation::animation(){
	m_animation_pos  = 0;     // pointer im wheel
};
animation::~animation(){};

uint8_t animation::get_key(){
	return ' ';
}

bool animation::init(){
}


bool animation::loop(){
	// // RGB circle ////
	if (m_animation_type.get_value()) { // if animation is not off (AKA 0)
		if (millis() >= m_animation_dimm_time) { // if it is time to animate
			if (neo) {
				if (m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL) {
					for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
						// TODO direct access to strip?
						strip.SetPixelColor(i, Wheel(((i * 256 / NEOPIXEL_LED_COUNT) + m_animation_pos) & 255));
					}
					m_animation_pos++; // move wheel by one, will overrun and therefore cycle
					strip.Show();
					m_animation_dimm_time = millis() + ANIMATION_STEP_TIME; // schedule update
				} else if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
					for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
						strip.SetPixelColor(i, Wheel(m_animation_pos & 255));
					}
					m_animation_pos++; // move wheel by one, will overrun and therefore cycle
					strip.Show();
					m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME; // schedule update
				} else if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
					// m_animation_pos geht bis 255/25 pixel = 10 colors,das wheel hat 255 pos, also 25 per 10 .. heh?
					strip.SetPixelColor(m_animation_pos % NEOPIXEL_LED_COUNT,
					  Wheel(int(m_animation_pos / NEOPIXEL_LED_COUNT) * 76 & 255)); // max: 255/10=25,25*10 to scale, 250*3 *3 will jump to various colors
					m_animation_pos++;                                              // move wheel by one, will overrun and therefore cycle
					strip.Show();
					m_animation_dimm_time = millis() + 3 * ANIMATION_STEP_TIME; // schedule update
				}
			} else if (bOne) {
				// TODO or AI?
				if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
					// TODO direct access?
					_my9291.setColor((my9291_color_t) { Wheel(m_animation_pos & 255).R, Wheel(m_animation_pos & 255).G,
					                                    Wheel(m_animation_pos & 255).B, 0, 0 }); // last two: warm white, cold white
					m_animation_pos++;                                                           // move wheel by one, will overrun and therefore cycle
					m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME;                  // schedule update

					/*
					 *   m_animation_pos=(m_animation_pos+1)%sizeof(hb);  // move wheel by one, will overrun and therefore cycle
					 *   _my9291.setColor((my9291_color_t) { intens[hb[m_animation_pos]], 0, 0, 0, 0 }); // last two: warm white, cold white
					 * m_animation_dimm_time = millis() + 3 * ANIMATION_STEP_TIME;                  // schedule update
					 */
				}
			}
		} // timer
		return false;// no un-interrupted execution required
	}  // if active
	// // RGB circle ////
	bool false; // i did nothing
}

bool animation::intervall_update(){
	return false;
}

bool animation::subscribe(){
	client.subscribe(build_topic(MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC)); // simple rainbow  topic
	client.loop();
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC), COLOR_GREEN);

	client.subscribe(build_topic(MQTT_RAINBOW_COMMAND_TOPIC)); // rainbow  topic
	client.loop();
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_RAINBOW_COMMAND_TOPIC), COLOR_GREEN);

	client.subscribe(build_topic(MQTT_COLOR_WIPE_COMMAND_TOPIC)); // color WIPE topic
	client.loop();
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_COLOR_WIPE_COMMAND_TOPIC), COLOR_GREEN);
	return true;
}

bool animation::receive(uint8_t* p_topic, uint8_t* p_payload){
	// //////////////////////// SET RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp(p_topic, build_topic(MQTT_RAINBOW_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_RAINBOW_WHEEL) {
			setAnimationType(ANIMATION_RAINBOW_WHEEL); // triggers also the publishing
		} else if (!strcmp_P((const char *) p_payload,
		   STATE_OFF) && m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL)
		{
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			// but only if the last pubish is less then 3 sec ago
			if (timer_republish_avoid + REPUBISH_AVOID_TIMEOUT < millis()) {
				m_animation_type.outdated();
			}
		}
	}
	// //////////////////////// SET RAINBOW /////////////////
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp(p_topic, build_topic(MQTT_SIMPLE_RAINBOW_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_RAINBOW_SIMPLE) {
			setAnimationType(ANIMATION_RAINBOW_SIMPLE);
		} else if (!strcmp_P((const char *) p_payload,
		   STATE_OFF) && m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE)
		{
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			// but only if the last pubish is less then 3 sec ago
			if (timer_republish_avoid + REPUBISH_AVOID_TIMEOUT < millis()) {
				m_animation_type.outdated();
			}
		}
	}
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp(p_topic, build_topic(MQTT_COLOR_WIPE_COMMAND_TOPIC))) {
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_COLOR_WIPE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF) && m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			// but only if the last pubish is less then 3 sec ago
			if (timer_republish_avoid + REPUBISH_AVOID_TIMEOUT < millis()) {
				m_animation_type.outdated();
			}
		}
	}
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////

	return false; // not for me
}


bool animation::publish(){
	// function called to publish the state of the rainbow (on/off)
	boolean ret = true;

	logger.print(TOPIC_MQTT_PUBLISH, F("rainbow state "), COLOR_GREEN);
	if (m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL) {
		Serial.println(STATE_ON);
		ret &= client.publish(build_topic(MQTT_RAINBOW_STATUS_TOPIC), STATE_ON, true);
	} else {
		Serial.println(STATE_OFF);
		ret &= client.publish(build_topic(MQTT_RAINBOW_STATUS_TOPIC), STATE_OFF, true);
	}

	logger.print(TOPIC_MQTT_PUBLISH, F("simple rainbow state "), COLOR_GREEN);
	if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
		Serial.println(STATE_ON);
		ret &= client.publish(build_topic(MQTT_SIMPLE_RAINBOW_STATUS_TOPIC), STATE_ON, true);
	} else {
		Serial.println(STATE_OFF);
		ret &= client.publish(build_topic(MQTT_SIMPLE_RAINBOW_STATUS_TOPIC), STATE_OFF, true);
	}

	logger.print(TOPIC_MQTT_PUBLISH, F("color WIPE state "), COLOR_GREEN);
	if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
		Serial.println(STATE_ON);
		ret &= client.publish(build_topic(MQTT_COLOR_WIPE_STATUS_TOPIC), STATE_ON, true);
	} else {
		Serial.println(STATE_OFF);
		ret &= client.publish(build_topic(MQTT_COLOR_WIPE_STATUS_TOPIC), STATE_OFF, true);
	}

	if (ret) {
		m_animation_type.outdated(false);
	}
	return ret;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor animation::wheel(byte WheelPos){
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


// set rainbow start point .. thats pretty much it
void setAnimationType(int type){
	// if we're starting a animation and have been on pwm light, switch pwm off
	if (m_pwm_light_state.get_value() && type != ANIMATION_OFF) {
		m_pwm_light_state.set(false);        // triggers update
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
		if (cap.m_use_neo_as_rgb.active) {
			for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
				strip.SetPixelColor(i, RgbColor(0, 0, 0));
			}
			strip.Show();
		} else if (cap.m_use_my92x1_as_rgb.active) {
			_my9291.setColor((my9291_color_t) { 0, 0, 0, 0, 0 }); // lights off
		}
	}
}
