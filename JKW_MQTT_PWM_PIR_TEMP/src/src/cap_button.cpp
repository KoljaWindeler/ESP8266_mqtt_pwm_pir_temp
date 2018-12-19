#include <cap_button.h>
#ifdef WITH_BUTTON

button::button(){
	m_pin = BUTTON_INPUT_PIN;
	m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
	m_polarity = LOW; // active low
	m_timer_button_down=0;
	m_counter=0;
};

button::~button(){
	detachInterrupt(digitalPinToInterrupt(m_pin));
	logger.println(TOPIC_GENERIC_INFO, F("Button deleted"), COLOR_YELLOW);
};


bool button::parse(uint8_t* config){
	// taster //
	if(cap.parse(config,get_key())){
		m_pin = BUTTON_INPUT_PIN;
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		return true;
	} else if(cap.parse_wide(config,get_key(),&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		return true;
	} else if(cap.parse_wide(config,(uint8_t*)"BN",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		return true;
	} else if(cap.parse_wide(config,(uint8_t*)"BP",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = HIGH; // active high !!
		return true;
	}
	// taster //

	// switch/umschalter //
	else if(cap.parse_wide(config,(uint8_t*)"BS",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_SWITCH; // switch/umschalter
		m_polarity = LOW; // active low
		return true;
	} else if(cap.parse_wide(config,(uint8_t*)"BSN",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_SWITCH; // switch/umschalter
		m_polarity = LOW; // active low
		return true;
	} else if(cap.parse_wide(config,(uint8_t*)"BSP",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_SWITCH; // switch/umschalter
		m_polarity = HIGH; // active high !!
		return true;
	}
	// switch/umschalter //

	return false;
}

uint8_t* button::get_key(){
	return (uint8_t*)"B";
}

void fooButton(){
	if(p_button){
		((button*)p_button)->interrupt();
	}
}

bool button::init(){
	logger.print(TOPIC_GENERIC_INFO, F("Button init: "), COLOR_GREEN);
	pinMode(m_pin, INPUT);

	// mode
	if(m_mode_toggle_switch == BUTTON_MODE_SWITCH){
		sprintf_P(m_msg_buffer,PSTR("switch on GPIO %i "),m_pin);
		pinMode(m_pin,INPUT); // disable pull up, shelly seams to have a super low curent driver
	} else {
		sprintf_P(m_msg_buffer,PSTR("push button on GPIO %i "),m_pin);
		pinMode(m_pin,INPUT_PULLUP); // pull up to avoid interrupts without sensor
	}
	logger.p(m_msg_buffer);

	// polarity
	if(m_polarity == LOW){
		sprintf_P(m_msg_buffer,PSTR("low active"));
	} else {
		sprintf_P(m_msg_buffer,PSTR("active high"));
	}
	logger.pln(m_msg_buffer);

	// attache interrupt codepwm for button
	attachInterrupt(digitalPinToInterrupt(m_pin), fooButton, CHANGE);

	// run wifi manager if a push button / taster is installed and hold down during init
	if (digitalRead(m_pin) == m_polarity && m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON) {
		wifiManager.startConfigPortal(CONFIG_SSID);
	}
	m_timer_checked=0;
}


bool button::loop(){
	/// start wifimanager config portal if we pushed the button more than 10 times in a row ///
	if (m_counter >= 10 && millis() - m_timer_button_down > BUTTON_TIMEOUT) {
		Serial.println(F("[SYS] Rebooting to setup mode"));
		delay(200);
		wifiManager.startConfigPortal(CONFIG_SSID);
	}

	// check once every 1s if the push-button is down and if so for how long
	if(m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON &&
			(millis()-m_timer_checked > BUTTON_CHECK_INTERVALL)){
		m_timer_checked=millis();
		// if button is pushed
		if (digitalRead(m_pin) == m_polarity) {
			for(int i=3;i>0;i--){
				// if the button is down for more then n seconds, set the state accordingly
				if(millis()-m_timer_button_down>i*BUTTON_LONG_PUSH){
					m_state.check_set(i); // will be send by the publish() routine
					break;
				}
			}
		}
	}
	return false; // i did nothing that should be none interrupted
}


bool button::publish(){
	if (m_state.get_outdated()) {
		boolean ret = false;
		if(m_state.get_value()==0){ // right after push
			logger.println(TOPIC_MQTT_PUBLISH, F("button push"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_0S,UNIT_TO_PC), (char*)MQTT_BUTTON_TOPIC_0S);
		} else if(m_state.get_value()==1){ // after 1 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 1s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_1S,UNIT_TO_PC), (char*)MQTT_BUTTON_TOPIC_1S);
		} else if(m_state.get_value()==2){ // after 2 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 2s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_2S,UNIT_TO_PC), (char*)MQTT_BUTTON_TOPIC_2S);
		} else if(m_state.get_value()==3){ // after 3 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 3s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_3S,UNIT_TO_PC), (char*)MQTT_BUTTON_TOPIC_3S);
		}
		else if(m_state.get_value() > BUTTON_RELEASE_OFFSET){ // release after > 1 sec (m_state == 10 + sec hold)
			sprintf(m_msg_buffer, "%i", m_state.get_value()-BUTTON_RELEASE_OFFSET);
			logger.print(TOPIC_MQTT_PUBLISH, F("button release after "), COLOR_GREEN);
			logger.pln(m_msg_buffer);
			ret = network.publish(build_topic(MQTT_BUTTON_RELEASE_TOPIC,UNIT_TO_PC), m_msg_buffer);
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}
	return false;
}

// external button push
void button::interrupt(){
	// toggle, write to pin, publish to server. For Push button: only once we have the "active" polarity, for SWITCH enter it every time
	if (digitalRead(m_pin) == m_polarity || m_mode_toggle_switch == BUTTON_MODE_SWITCH) {
		if (millis() - m_timer_button_down > BUTTON_DEBOUNCE) { // avoid bouncing
			// toggle status of both lights
			if(p_light){
				((light*)p_light)->toggle();
			}

			if(m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON){ // Push-Button (Taster): Count pushes (m_counter) within distance < BUTTON_TIMEOUT
				// button down, m_state saves button push time, the "has been hold for n-second" is in the loop() routine
				m_state.set(0);

				// keep counting if we're fast enough
				if (millis() - m_timer_button_down < BUTTON_TIMEOUT) {
					m_counter++;
				} else {
					m_counter = 1;
				}
				logger.print(TOPIC_BUTTON, F("push nr "));
				Serial.println(m_counter);
			}
		};
		// Serial.print(".");
	} else {
		// RELEASE push - button after holding for n*BUTTON_LONG_PUSH (1000ms), SWITCH never gets here
		if(millis() - m_timer_button_down > BUTTON_LONG_PUSH) {
			// set the state to BUTTON_RELEASE_OFFSET (10) + N, where N = how many we've hold down the key
			m_state.set( BUTTON_RELEASE_OFFSET + (millis() - m_timer_button_down) / BUTTON_LONG_PUSH );
		}
	};
	m_timer_button_down = millis(); // avoid bouncing, and recogize the state changes (Push-Button will count hold seconds, based on this)
}

#endif
