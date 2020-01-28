#include <cap_light.h>

light::light(){
	m_light_target  = { 99, 99, 99 };
	m_light_start   = { 99, 99, 99 };
	m_light_current = { 99, 99, 99 }; // actual value written to PWM
	m_light_backup  = { 99, 99, 99 }; // to be able to resume "dimm on" to the last set color
	m_pwm_dimm_time = 10;             // 10ms per Step, 255*0.01 = 2.5 sec
};
light::~light(){
#ifdef WITH_DISCOVERY
	if(timer_connected_start>0){
		network.publish(build_topic(MQTT_LIGHT_TOPIC,UNIT_TO_PC), "");
		network.publish(build_topic(MQTT_LIGHT_BRIGHTNESS_TOPIC,UNIT_TO_PC), "");
		network.publish(build_topic(MQTT_LIGHT_COLOR_TOPIC,UNIT_TO_PC), "");
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("light deleted"), COLOR_YELLOW);
};

uint8_t * light::get_key(){
	return (uint8_t*)"LIG";
}


bool light::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

bool light::init(){
	timer_dimmer       = 0;
	timer_dimmer_start = 0;
	timer_dimmer_end   = 0;
	m_onfor_offtime		 = 0;
	type = 0;
	m_light_brightness.set(99,false); // init values, will be overwirtten by MQTT commands
	m_animation_brightness.set(99,false);

	provider[0] = NULL;
	provider[1] = NULL;
	provider[2] = NULL;

	logger.println(TOPIC_GENERIC_INFO, F("light init"), COLOR_GREEN);
	return true;
}

bool light::reg_provider(capability * p, uint8_t* t){
	uint8_t free_slot = 0;
	while((free_slot<MAX_LIGHT_PROVIDER) & (provider[free_slot]!=NULL)){
		free_slot++;
	}
	provider[free_slot] = (light_providing_capability*)p;
	logger.print(TOPIC_GENERIC_INFO, F("light registered provider "), COLOR_GREEN);
	logger.p(free_slot);
	logger.p(": ");
	provider[free_slot]->print_name();
	return false;
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
		}
		m_light_current = (led){ 0, 0, 0 };
		send_current_light();
	}
}

void light::setColor(uint8_t r, uint8_t g, uint8_t b){
	uint32_t c=((uint32_t)r<<8)+((uint32_t)g<<4)+b;
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
		m_light_target = m_light_backup; // not sure if this is needed
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
		//logger.p(".");
		if (millis()-timer_dimmer >= m_pwm_dimm_time) {
			// logger.p("DIMMER ");
			timer_dimmer = millis(); // save for next round

			// set new value
			if (timer_dimmer + m_pwm_dimm_time > timer_dimmer_end) {
				m_light_current = m_light_target;
				timer_dimmer_end = 0; // last step, stop dimming
			} else {
				m_light_current.r = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.r, m_light_target.r);
				m_light_current.g = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.g, m_light_target.g);
				m_light_current.b = map(timer_dimmer, timer_dimmer_start, timer_dimmer_end, m_light_start.b, m_light_target.b);
			}
			//logger.pln(m_light_current.r);``
			send_current_light();
		}
		return true; // muy importante .. request uninterrupted execution
	}

	// // RGB circle ////
	if (m_animation_type.get_value()) {
		if (millis() >= m_animation_dimm_time) {
			for(uint8_t provider_slot=0; (provider_slot<MAX_LIGHT_PROVIDER) & (provider[provider_slot]!=NULL); provider_slot++){
				if (provider[provider_slot]->get_modes() & (1<<SUPPORTS_PER_PIXEL)) {
					if (m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL) {
						for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
							RgbColor c = Wheel(((i * 256 / NEOPIXEL_LED_COUNT) + m_animation_pos) & 255);
							c.R = (((uint16_t)c.R)*m_animation_brightness.get_value())/99;
							c.G = (((uint16_t)c.G)*m_animation_brightness.get_value())/99;
							c.B = (((uint16_t)c.B)*m_animation_brightness.get_value())/99;
							provider[provider_slot]->set_color(c.R, c.G, c.B, i);
						}
						m_animation_pos++; // move wheel by one, will overrun and therefore cycle
						provider[provider_slot]->show();
						m_animation_dimm_time = millis() + ANIMATION_STEP_TIME; // schedule update
					} else if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
						RgbColor c = Wheel(m_animation_pos & 255);
						c.R = (((uint16_t)c.R)*m_animation_brightness.get_value())/99;
						c.G = (((uint16_t)c.G)*m_animation_brightness.get_value())/99;
						c.B = (((uint16_t)c.B)*m_animation_brightness.get_value())/99;
						for (int i = 0; i < NEOPIXEL_LED_COUNT; i++) {
							provider[provider_slot]->set_color(c.R, c.G, c.B, i);
						}
						m_animation_pos++; // move wheel by one, will overrun and therefore cycle
						provider[provider_slot]->show();
						m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME; // schedule update
					} else if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
						// m_animation_pos geht bis 255/25 pixel = 10 colors,das wheel hat 255 pos, also 25 per 10 .. heh?
						RgbColor c = Wheel(int(m_animation_pos / NEOPIXEL_LED_COUNT) * 76 & 255);
						c.R = (((uint16_t)c.R)*m_animation_brightness.get_value())/99;
						c.G = (((uint16_t)c.G)*m_animation_brightness.get_value())/99;
						c.B = (((uint16_t)c.B)*m_animation_brightness.get_value())/99;
						provider[provider_slot]->set_color(c.R, c.G, c.B, m_animation_pos % NEOPIXEL_LED_COUNT);
						// max: 255/10=25,25*10 to scale, 250*3 *3 will jump to various colors
						m_animation_pos++; // move wheel by one, will overrun and therefore cycle
						provider[provider_slot]->show();
						m_animation_dimm_time = millis() + 3 * ANIMATION_STEP_TIME; // schedule update
					}
				} else if (provider[provider_slot]->get_modes() & (1<<SUPPORTS_PWM)) {
					if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
						RgbColor c = Wheel(m_animation_pos & 255);
						c.R = (((uint16_t)c.R)*m_animation_brightness.get_value())/99;
						c.G = (((uint16_t)c.G)*m_animation_brightness.get_value())/99;
						c.B = (((uint16_t)c.B)*m_animation_brightness.get_value())/99;
						provider[provider_slot]->set_color(c.R, c.G, c.B, 255);                          // last two: warm white, cold white
						m_animation_pos++;                                          // move wheel by one, will overrun and therefore cycle
						m_animation_dimm_time = millis() + 4 * ANIMATION_STEP_TIME; // schedule update
					} else if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
						// m_animation_pos geht bis 255/25 pixel = 10 colors,das wheel hat 255 pos, also 25 per 10 .. heh?
						RgbColor c = Wheel(int(m_animation_pos / NEOPIXEL_LED_COUNT) * 76 & 255);
						c.R = (((uint16_t)c.R)*m_animation_brightness.get_value())/99;
						c.G = (((uint16_t)c.G)*m_animation_brightness.get_value())/99;
						c.B = (((uint16_t)c.B)*m_animation_brightness.get_value())/99;
						if(m_animation_pos % NEOPIXEL_LED_COUNT == 0){
							provider[provider_slot]->set_color(c.R, c.G, c.B, 255);                          // last two: warm white, cold white
						}
						// max: 255/10=25,25*10 to scale, 250*3 *3 will jump to various colors
						m_animation_pos++; // move wheel by one, will overrun and therefore cycle
						m_animation_dimm_time = millis() + 3 * ANIMATION_STEP_TIME; // schedule update
					}
				}
			}
		} // timer
	}  // if active
	   // // RGB circle ////
	// auto off
	if(m_onfor_offtime != 0){
		if(m_onfor_offtime < millis()){
			// unset
			m_onfor_offtime = 0;
			// switch of
			m_state.set(false);
			m_light_backup  = m_light_target; // save last target value to resume later on
			m_light_current = (led){ 0, 0, 0 };
			send_current_light();
		}
	}
	return false; // i did nothing
} // loop

// bool light::intervall_update(uint8_t slot){  return false; }

bool light::subscribe(){
	network.subscribe(build_topic(MQTT_LIGHT_TOPIC,PC_TO_UNIT)); // on off
	logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_TOPIC,PC_TO_UNIT), COLOR_GREEN);

	uint8_t mvp=0;
	for(uint8_t provider_slot=0; (provider_slot<MAX_LIGHT_PROVIDER) & (provider[provider_slot]!=NULL); provider_slot++){
		if(provider[provider_slot]->get_modes()>mvp){
			mvp = provider[provider_slot]->get_modes();
		}
	}
	if (mvp&(1<<SUPPORTS_PWM)) { // pwm, neo, b1, AI
		////////////////// color topic, ////////////////////////////////
		//subscribe this before brightness topic
		network.subscribe(build_topic(MQTT_LIGHT_COLOR_TOPIC,PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_COLOR_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		network.subscribe(build_topic(MQTT_LIGHT_DIMM_COLOR_TOPIC,PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_COLOR_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		////////////////// brightness topic, ////////////////////////////////
		//subscribe this before the on off!
		network.subscribe(build_topic(MQTT_LIGHT_BRIGHTNESS_TOPIC,PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_BRIGHTNESS_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		network.subscribe(build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_TOPIC,PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		////////////////// dimm topic, ////////////////////////////////
		// on /off topics
		network.subscribe(build_topic(MQTT_LIGHT_DIMM_TOPIC,PC_TO_UNIT)); // dimm on
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		network.subscribe(build_topic(MQTT_LIGHT_DIMM_DELAY_TOPIC,PC_TO_UNIT));
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_DIMM_DELAY_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	}
	if (mvp&(1<<SUPPORTS_RGB)) { // neo, b1, ai
		network.subscribe(build_topic(MQTT_LIGHT_ANIMATION_BRIGHTNESS_TOPIC,PC_TO_UNIT)); // animation brightness topic
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_BRIGHTNESS_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		network.subscribe(build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC,PC_TO_UNIT), COLOR_GREEN);

		network.subscribe(build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC,PC_TO_UNIT)); // color WIPE topic
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	}

	if (mvp&(1<<SUPPORTS_PER_PIXEL)) {
		network.subscribe(build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC,PC_TO_UNIT)); // rainbow  topic
		logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	}

	return true;
} // subscribe

bool light::receive(uint8_t * p_topic, uint8_t * p_payload){
	// dimm to given PWM value
	if ( (!strcmp((const char *) p_topic,
	    build_topic(MQTT_LIGHT_TOPIC,PC_TO_UNIT))) ||
	  (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_TOPIC,PC_TO_UNIT)))  ) {
		m_onfor_offtime = 0;
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) || !strncmp_P((const char*) p_payload, STATE_ONFOR, 5)) {
			if (m_state.get_value() != true || m_animation_type.get_value()) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) { // if there is an animation, switch it off
					setAnimationType(ANIMATION_OFF);
				}

				// bei diesem topic hartes einschalten
				m_light_current = m_light_backup;
				send_current_light();
				// Home Assistant will assume that the pwm light is 100%, once we "turn it on"
				// but it should return to whatever the m_light_brithness is
				m_state.set(true);
				m_light_brightness.outdated();
				m_light_color.outdated(); // set this to publish that we've left the color
			} else {
				// was already on .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}

			// only leave the light on for some seconds
			if(!strncmp_P((const char*) p_payload, STATE_ONFOR, 5)){
				m_onfor_offtime = millis() + 1000 * atoi((const char*) (p_payload+5));
				logger.print(TOPIC_GENERIC_INFO, F("Auto off in "),COLOR_PURPLE);
				sprintf(m_msg_buffer,"%lu sec",(m_onfor_offtime-millis())/1000+1);
				logger.pln(m_msg_buffer);
			}
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
			if (m_state.get_value() != false  || m_animation_type.get_value()) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) { // if there is an animation, switch it off
					setAnimationType(ANIMATION_OFF);
				}

				m_state.set(false);
				m_light_backup  = m_light_target; // save last target value to resume later on
				m_light_current = (led){ 0, 0, 0 };
				send_current_light();
			} else {
				// was already off .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		}
		return true; // command consumed
	}
	// dimm to given PWM value
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		logger.println(TOPIC_MQTT, F("received dimm command"),COLOR_PURPLE);
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON)) {
			if (m_state.get_value() != true || m_animation_type.get_value()) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) {
					setAnimationType(ANIMATION_OFF);
				}
				// logger.pln("light was off");
				m_state.set(true);
				m_light_brightness.outdated();
				m_light_color.outdated(); // set this to publish that we've left the color
				// bei diesem topic ein dimmen
				DimmTo(m_light_backup);
			} else {
				// was already on .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) {
			if (m_state.get_value() != false || m_animation_type.get_value()) {
				// we don't want to keep running the animation if brightness was submitted
				if (m_animation_type.get_value()) {
					setAnimationType(ANIMATION_OFF);
				}
				// logger.pln("light was on");
				m_state.set(false);
				//m_light_color.set(m_light_target.r + m_light_target.g + m_light_target.b); // set this to publish that we've left the color
				// remember the current target value to resume to this value once we receive a 'dimm on 'command
				m_light_backup = m_light_target; // target instead off current (just in case we are currently dimming)
				DimmTo((led){ 0, 0, 0 });        // dimm off
			} else {
				// was already off .. and the received didn't know it .. so we have to re-publish
				m_state.outdated();
			}
		}
		return true; // command consumed
	}
	// //////////////////////// SET RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC,PC_TO_UNIT))) {
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
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
		return true; // command consumed
	}
	// //////////////////////// SET RAINBOW /////////////////
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC,PC_TO_UNIT))) {
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
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
		return true; // command consumed
	}
	// //////////////////////// SET SIMPLE RAINBOW /////////////////
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////
	// simply switch GPIO ON/OFF
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC,PC_TO_UNIT))) {
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		// test if the payload is equal to "ON" or "OFF"
		if (!strcmp_P((const char *) p_payload, STATE_ON) && m_animation_type.get_value() != ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_COLOR_WIPE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF) && m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
			setAnimationType(ANIMATION_OFF);
		} else {
			// was already in the state .. and the received didn't know it .. so we have to re-publish
			m_animation_type.outdated();
		}
		return true; // command consumed
	}
	// //////////////////////// SET SIMPLE COLOR WIPE /////////////////
	// //////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
	else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_ANIMATION_BRIGHTNESS_TOPIC,PC_TO_UNIT))) { // directly set the color, hard
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		uint8_t t = (uint8_t) atoi((const char *) p_payload);
		if(t>=100){
			t=99;
		}
		m_animation_brightness.check_set(t); // will also eventually trigger publish
		return true; // command consumed
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_BRIGHTNESS_TOPIC,PC_TO_UNIT))) { // directly set the color, hard
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		uint8_t t = (uint8_t) atoi((const char *) p_payload);

		if(m_state.get_value()){
			// we don't want to keep running the animation if brightness was submitted
			if (m_animation_type.get_value()) { // if there is an animation, switch it off
				setAnimationType(ANIMATION_OFF);
			}
			m_light_current.r = t;                                          // das regelt die helligkeit
			m_light_current.g = t;                                          // das regelt die helligkeit
			m_light_current.b = t;                                          // das regelt die helligkeit
			m_light_target = m_light_current;

			// publish
			m_light_brightness.set(t);
			m_light_color.outdated(); // needed to update HA symbol

			send_current_light();
		} else {
			m_light_backup.r = t;                                          // das regelt die helligkeit
			m_light_backup.g = t;                                          // das regelt die helligkeit
			m_light_backup.b = t;                                          // das regelt die helligkeit
		}
		return true; // command consumed
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_BRIGHTNESS_TOPIC,PC_TO_UNIT))) { // smooth dimming of pwm
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		uint8_t t = (uint8_t) atoi((const char *) p_payload);
		if(m_state.get_value()){
			// we don't want to keep running the animation if brightness was submitted
			if (m_animation_type.get_value()) { // if there is an animation, switch it off
				setAnimationType(ANIMATION_OFF);
			}
			logger.print(TOPIC_MQTT, F("dimm brightness command "),COLOR_PURPLE);
			logger.pln(t);
			// publish
			m_light_brightness.set(t);
			m_light_color.outdated(); // needed to update HA symbol

			DimmTo((led){ t, t, t });
		} else {
			m_light_backup.r = t;                                          // das regelt die helligkeit
			m_light_backup.g = t;                                          // das regelt die helligkeit
			m_light_backup.b = t;                                          // das regelt die helligkeit
		}
		return true; // command consumed
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_COLOR_TOPIC,PC_TO_UNIT))) { // directly set rgb, hard
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		uint8_t color[3] = { 0, 0, 0 };
		uint8_t s        = 0;
		for (uint8_t i = 0; p_payload[i]; i++) {
			if (p_payload[i] == ',') {
				s = (s + 1) % 3;
			} else {
				color[s] = color[s] * 10 + p_payload[i] - '0';
			}
		}
		if(m_state.get_value()){
			// we don't want to keep running the animation if brightness was submitted
			if (m_animation_type.get_value()) { // if there is an animation, switch it off
				setAnimationType(ANIMATION_OFF);
			}

			logger.println(TOPIC_MQTT, F("hard color command"),COLOR_PURPLE);
			m_light_color.outdated(); // color will be used directly
			m_light_brightness.outdated();
			// HA is sending color as 0-255 where as we want it 0-99
			m_light_current = (led){
				(uint8_t) map(color[0], 0, 255, 0, 99),
				(uint8_t) map(color[1], 0, 255, 0, 99),
				(uint8_t) map(color[2], 0, 255, 0, 99)
			};
			m_light_target = m_light_current;
			send_current_light();
		} else {
			m_light_backup = (led){
				(uint8_t) map(color[0], 0, 255, 0, 99),
				(uint8_t) map(color[1], 0, 255, 0, 99),
				(uint8_t) map(color[2], 0, 255, 0, 99)
			};
		}
		return true; // command consumed
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_COLOR_TOPIC,PC_TO_UNIT))) { // smoothly dimm to rgb value
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		uint8_t color[3] = { 0, 0, 0 };
		uint8_t s        = 0;
		for (uint8_t i = 0; p_payload[i]; i++) {
			if (p_payload[i] == ',') {
				s = (s + 1) % 3;
			} else {
				color[s] = color[s] * 10 + p_payload[i] - '0';
			}
		}

		if(m_state.get_value()){
			// we don't want to keep running the animation if brightness was submitted
			if (m_animation_type.get_value()) { // if there is an animation, switch it off
				setAnimationType(ANIMATION_OFF);
			}
			logger.println(TOPIC_MQTT, F("color dimm command"),COLOR_PURPLE);
			m_light_color.outdated(); // color will be used directly
			m_light_brightness.outdated();
			// input color values are 888rgb .. DimmTo needs precentage which it will convert it into half log brighness
			DimmTo((led){
			   (uint8_t) map(color[0], 0, 255, 0, 99),
			   (uint8_t) map(color[1], 0, 255, 0, 99),
			   (uint8_t) map(color[2], 0, 255, 0, 99)
					});
		} else {
			m_light_backup = (led){
				(uint8_t) map(color[0], 0, 255, 0, 99),
				(uint8_t) map(color[1], 0, 255, 0, 99),
				(uint8_t) map(color[2], 0, 255, 0, 99)
			};
		}
		return true; // command consumed
	} else if (!strcmp((const char *) p_topic, build_topic(MQTT_LIGHT_DIMM_DELAY_TOPIC,PC_TO_UNIT))) { // adjust dimmer delay
		m_onfor_offtime = 0; // avoid delay auto off switching if we get some other commands in between
		m_pwm_dimm_time = atoi((const char *) p_payload);
		// logger.p("Setting dimm time to: ");
		// logger.pln(m_pwm_dimm_time);
		return true; // command consumed
	}
	// if we didn't use the command ..
	return false; // not for me
} // receive

bool light::publish(){
	if (!publishLightState()) {
		if (!publishRGBColor()) {
			if (!publishLightBrightness()) {
				if(!publishAnimationType()){
					if(!publishAnimationBrightness()){
						return false;
					}
				}
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
			logger.pln((char*)STATE_ON);
			ret = network.publish(build_topic(MQTT_LIGHT_TOPIC,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			logger.pln((char*)STATE_OFF);
			ret = network.publish(build_topic(MQTT_LIGHT_TOPIC,UNIT_TO_PC), (char*)STATE_OFF);
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
		uint8_t v = 0;
		if(m_light_target.r == m_light_target.g && m_light_target.r==m_light_target.b){
			v=m_light_target.r;
		}
		logger.print(TOPIC_MQTT_PUBLISH, F("light brightness "), COLOR_GREEN);
		logger.pln(v);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", v);
		ret = network.publish(build_topic(MQTT_LIGHT_BRIGHTNESS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_light_brightness.set(v, false); // required?
		}
		return ret;
	}
	return false;
}

// function called to publish the brightness of the led
bool light::publishAnimationBrightness(){
	if (m_animation_brightness.get_outdated()) {
		boolean ret = false;
		logger.print(TOPIC_MQTT_PUBLISH, F("animation brightness "), COLOR_GREEN);
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d", m_animation_brightness.get_value());
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_LIGHT_ANIMATION_BRIGHTNESS_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_animation_brightness.outdated(false);
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
		snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%d,%d,%d", (uint8_t) map(m_light_target.r, 0, 99, 0, 255),(uint8_t) map(m_light_target.g, 0, 99, 0, 255), (uint8_t) map(m_light_target.b, 0, 99, 0, 255));
		logger.pln(m_msg_buffer);
		ret = network.publish(build_topic(MQTT_LIGHT_COLOR_TOPIC,UNIT_TO_PC), m_msg_buffer);
		if (ret) {
			m_light_color.outdated(false);
		}
		return ret;
	}
	return false;
}

bool light::publishAnimationType(){
	if (m_animation_type.get_outdated()) {
		boolean ret = true;

		logger.print(TOPIC_MQTT_PUBLISH, F("rainbow state "), COLOR_GREEN);
		if (m_animation_type.get_value() == ANIMATION_RAINBOW_WHEEL) {
			Serial.println(STATE_ON);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			Serial.println(STATE_OFF);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_RAINBOW_TOPIC,UNIT_TO_PC), (char*)STATE_OFF);
		}

		logger.print(TOPIC_MQTT_PUBLISH, F("simple rainbow state "), COLOR_GREEN);
		if (m_animation_type.get_value() == ANIMATION_RAINBOW_SIMPLE) {
			Serial.println(STATE_ON);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			Serial.println(STATE_OFF);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_SIMPLE_RAINBOW_TOPIC,UNIT_TO_PC), (char*)STATE_OFF);
		}

		logger.print(TOPIC_MQTT_PUBLISH, F("color WIPE state "), COLOR_GREEN);
		if (m_animation_type.get_value() == ANIMATION_COLOR_WIPE) {
			Serial.println(STATE_ON);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			Serial.println(STATE_OFF);
			ret &= network.publish(build_topic(MQTT_LIGHT_ANIMATION_COLOR_WIPE_TOPIC,UNIT_TO_PC), (char*)STATE_OFF);
		}

		if (ret) {
			m_animation_type.outdated(false);
		}
		return ret;
	}
	return false;
} // publishAnimationType

void light::DimmTo(led dimm_to){
	logger.print(TOPIC_GENERIC_INFO, F("DimmTo "), COLOR_PURPLE);
	logger.p(dimm_to.r);
	logger.p(F(","));
	logger.p(dimm_to.g);
	logger.p(F(","));
	logger.pln(dimm_to.b);
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

	// logger.p("Enabled dimming, timing: ");
	// logger.pln(m_pwm_dimm_time);
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
		for(uint8_t provider_slot=0; (provider_slot<MAX_LIGHT_PROVIDER) & (provider[provider_slot]!=NULL); provider_slot++){
			provider[provider_slot]->set_color(0,0,0, 255);
		}
	}
}

void light::send_current_light(){
	led temp;
	temp.r = intens[_min(m_light_current.r,sizeof(intens)-1)];
	temp.g = intens[_min(m_light_current.g,sizeof(intens)-1)];
	temp.b = intens[_min(m_light_current.b,sizeof(intens)-1)];
	for(uint8_t provider_slot=0; (provider_slot<MAX_LIGHT_PROVIDER) & (provider[provider_slot]!=NULL); provider_slot++){
		provider[provider_slot]->set_color(temp.r, temp.g, temp.b, 255);
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
