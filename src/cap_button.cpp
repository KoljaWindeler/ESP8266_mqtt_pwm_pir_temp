#include <cap_button.h>
#ifdef WITH_BUTTON



button::button(){
	m_pin = BUTTON_INPUT_PIN;
	m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
	m_polarity = LOW; // active low
	m_pullup = true;
	m_timer_button_down=0;
	m_timer_debounce=0;
	m_counter=0;
	m_pin_active = false;
	m_discovery_pub = false;
	m_ghost_avoidance = 0;
	m_interrupt_ready = false;
	m_BL_conn = false;
};

button::~button(){
	detachInterrupt(digitalPinToInterrupt(m_pin));
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_B_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing B config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		delete[] t;

		char* t1s = discovery_topic_bake(MQTT_DISCOVERY_B1S_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing B1S config "), COLOR_YELLOW);
		logger.pln(t1s);
		network.publish(t1s,(char*)"");
		delete[] t1s;

		m_discovery_pub = false;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("Button deleted"), COLOR_YELLOW);
};


bool button::parse(uint8_t* config){
	// keyword "BLCONN" says that we  want to connect the button to the light
	// so triggering the button shall toggle the light
	if(cap.parse(config,(uint8_t*)"BLCONN")){
		m_BL_conn = true;
	}

	bool ret=false;
	// taster //
	if(cap.parse(config,get_key())){
		m_pin = BUTTON_INPUT_PIN;
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		m_pullup = true;
		ret = true;
	} else if(cap.parse_wide(config,get_key(),&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		m_pullup = true;
		ret = true;
	} else if(cap.parse_wide(config,(uint8_t*)"BN",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		m_pullup = true;
		ret = true;
	} else if(cap.parse_wide(config,(uint8_t*)"BNN",&m_pin)){ // very special for shelly, it switches to LOW but can't drive against the pullup
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = LOW; // active low
		m_pullup = false;
		ret = true;
	} else if(cap.parse_wide(config,(uint8_t*)"BP",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_PUSH_BUTTON; // taster
		m_polarity = HIGH; // active high !!
		m_pullup = false;
		ret = true;
	}
	// taster //

	// switch/umschalter //
	else if(cap.parse_wide(config,(uint8_t*)"BS",&m_pin)){
		m_mode_toggle_switch = BUTTON_MODE_SWITCH; // switch/umschalter
		m_polarity = LOW; // active low .. doesn't matter
		m_pullup = false;
		ret = true;
	}
	// switch/umschalter //

	if(ret){
		// ghost avoidance first, as it won't return //
		if(cap.parse(config,(uint8_t*)"GHOST")){
			m_ghost_avoidance = 50;
		};
		cap.parse_wide(config,"%s%i", (uint8_t*)"GHOST", 0, 255, &m_ghost_avoidance, (uint8_t*)"");
		if(m_ghost_avoidance){
			logger.print(TOPIC_GENERIC_INFO, F("Enable anti-ghosting "), COLOR_PURPLE);
			logger.pln(m_ghost_avoidance);
		}
	}

	return ret;
}

uint8_t* button::get_key(){
	return (uint8_t*)"B";
}

void ICACHE_RAM_ATTR fooButton(){
	if(p_button){
		((button*)p_button)->interrupt();
	}
}

void WifiButton(){
	if(p_button){
		((button*)p_button)->consume_interrupt();
	}
}

bool button::init(){
	logger.print(TOPIC_GENERIC_INFO, F("Button init: "), COLOR_GREEN);

	// mode
	if(m_pullup){
		pinMode(m_pin,INPUT_PULLUP); // pull up to avoid interrupts without sensor
	} else {
		pinMode(m_pin,INPUT); // pull up to avoid interrupts without sensor
	}

	if(m_mode_toggle_switch == BUTTON_MODE_SWITCH){
		sprintf_P(m_msg_buffer,PSTR("switch on GPIO %i "),m_pin);
		attachInterrupt(digitalPinToInterrupt(m_pin), fooButton, CHANGE);
		m_polarity = !digitalRead(m_pin); // predict next state
	} else {
		sprintf_P(m_msg_buffer,PSTR("push button on GPIO %i "),m_pin);
		logger.p(m_msg_buffer);
		// polarity
		if(m_polarity == LOW){
			attachInterrupt(digitalPinToInterrupt(m_pin), fooButton, FALLING);
			sprintf_P(m_msg_buffer,PSTR("low active"));
		} else {
			attachInterrupt(digitalPinToInterrupt(m_pin), fooButton, RISING);
			sprintf_P(m_msg_buffer,PSTR("active high"));
		}
	}
	logger.pln(m_msg_buffer);

	// run wifi manager if a push button / taster is installed and hold down during init
	if (digitalRead(m_pin) == m_polarity && m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON && m_pullup) {
		m_pin_active = true; // button is pushed
		wifiManager.startConfigPortal(CONFIG_SSID);
	}
	// loop timer, check instantly
	m_timer_checked=0;
	return true;
}


bool button::loop(){
	consume_interrupt();

	/// start wifimanager config portal if we pushed the button more than 10 times in a row ///
	if (m_counter >= 20 && millis() - m_timer_button_down > BUTTON_TIMEOUT) {
		Serial.println(F("[SYS] Rebooting to setup mode"));
		delay(200);
		wifiManager.startConfigPortal(CONFIG_SSID);
	}

	// check once every 1s if the push-button is down and if so for how long
	if(m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON &&
			(millis()-m_timer_checked > BUTTON_CHECK_INTERVALL)){
		m_timer_checked=millis();

		// if button is pushed
		if (digitalRead(m_pin) == m_polarity){
			// button is still pushed
			for(uint8_t i=3;i>0;i--){
				// if the button is down for more then n seconds, set the state accordingly
				if(millis()-m_timer_button_down>i*BUTTON_LONG_PUSH){
					m_state.check_set(i); // will be send by the publish() routine
					break;
				}
			}
			// keep checking faster during pin is down
			m_timer_checked = millis() - BUTTON_CHECK_INTERVALL + BUTTON_FAST_CHECK_INTERVALL;
			// if the push button is still pushed, avoid further triggerin with this
			// otherwise the release of the bouncing push button will trigger again
			m_timer_debounce = millis();
		}
		// push button is not loger pushed, but was previously, check if was hold for enough time to send a "release after xx msg"
		else if(m_pin_active){
			m_pin_active = false; // button released
			// RELEASE push - button after holding for n*BUTTON_LONG_PUSH (1000ms), SWITCH never gets here
			// set the state to BUTTON_RELEASE_OFFSET (10) + N, where N = how many sec we've hold down the key
			m_state.set( BUTTON_RELEASE_OFFSET + (millis() - m_timer_button_down) / BUTTON_LONG_PUSH );
			m_timer_button_down = millis();
		}
	}
	return false; // i did nothing that should be none interrupted
}


bool button::publish(){
	if (m_state.get_outdated()) {
		boolean ret = false;
		if(m_state.get_value()==0){ // right after push
			logger.println(TOPIC_MQTT_PUBLISH, F("button push"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_0S,UNIT_TO_PC), (char*)STATE_ON);
		} else if(m_state.get_value()==1){ // after 1 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 1s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_1S,UNIT_TO_PC), (char*)STATE_ON);
		} else if(m_state.get_value()==2){ // after 2 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 2s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_2S,UNIT_TO_PC), (char*)STATE_ON);
		} else if(m_state.get_value()==3){ // after 3 sec
			logger.println(TOPIC_MQTT_PUBLISH, F("button push 3s"), COLOR_GREEN);
			ret = network.publish(build_topic(MQTT_BUTTON_TOPIC_3S,UNIT_TO_PC), (char*)STATE_ON);
		} else {
			//released
			sprintf(m_msg_buffer, "%i", m_state.get_value()-BUTTON_RELEASE_OFFSET);
			logger.print(TOPIC_MQTT_PUBLISH, F("button release after "), COLOR_GREEN);
			logger.pln(m_msg_buffer);
			ret = network.publish(build_topic(MQTT_BUTTON_RELEASE_TOPIC,UNIT_TO_PC), m_msg_buffer);
			for(int8_t i=min(m_state.get_value()-BUTTON_RELEASE_OFFSET,3);i>=0;i--){
				if(i==3){
					ret &= network.publish(build_topic(MQTT_BUTTON_TOPIC_3S,UNIT_TO_PC), (char*)STATE_OFF);
				} else if(i==2){
					ret &= network.publish(build_topic(MQTT_BUTTON_TOPIC_2S,UNIT_TO_PC), (char*)STATE_OFF);
				} else if(i==1){
					ret &= network.publish(build_topic(MQTT_BUTTON_TOPIC_1S,UNIT_TO_PC), (char*)STATE_OFF);
				} else {
					ret &= network.publish(build_topic(MQTT_BUTTON_TOPIC_0S,UNIT_TO_PC), (char*)STATE_OFF);
				}
			}
		}
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}

#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_B_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_B_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_B_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("B discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;

			char* t1s = discovery_topic_bake(MQTT_DISCOVERY_B1S_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m1s = new char[strlen(MQTT_DISCOVERY_B1S_MSG)+2*strlen(mqtt.dev_short)];
			sprintf(m1s, MQTT_DISCOVERY_B1S_MSG, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("B1S discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = m_discovery_pub && network.publish(t1s,m1s);
			delete[] m1s;
			delete[] t1s;
			return true;
		}
	}
#endif

	return false;
}

bool button::consume_interrupt(){
	if(m_interrupt_ready){
		// processed
		m_interrupt_ready = false;

		// ghost switching avoidance, Shelly switches tend to see little peaks << 100ms as input
		if(m_ghost_avoidance){
			delay(m_ghost_avoidance);
			if(digitalRead(m_pin) != m_polarity){
				return false;
			}
		}

		// Push-Button (Taster)
		if(m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON){
			m_pin_active = true; // button is pushed
		} else {
			m_polarity = !m_polarity; // predict next change
		}

		// avoid bouncing
		if (millis() - m_timer_debounce > BUTTON_DEBOUNCE) {
			// toggle status of both lights, but only do that if there is a light //
			// and the NO_Button_to_light_connection is false (so they are actually connected)
			// or if we are offline (sort of an backup mechanism
			if(p_light && (m_BL_conn || !network.connected())){
				((light*)p_light)->toggle(true);
			}
			// keep counting if we're fast enough
			if (millis() - m_timer_button_down < BUTTON_TIMEOUT) {
				m_counter++;
			} else {
				m_counter = 1;
			}
			// check time fornext push
			m_timer_button_down = millis();
			// publish change, if this is a button always publish ON, as OFF will be handled in the loop,
			if(m_mode_toggle_switch == BUTTON_MODE_PUSH_BUTTON){
				m_state.set(0);
			} else { // but if it is a switch, we have to take care of polarity (0=ON, 10=OFF)
				m_state.set(m_polarity*BUTTON_RELEASE_OFFSET);
			}
			// make sure that the loop checks in 0.1s from now if the button is still down
			m_timer_checked = millis() - BUTTON_CHECK_INTERVALL + BUTTON_FAST_CHECK_INTERVALL;

			logger.print(TOPIC_BUTTON, F("push nr "));
			Serial.println(m_counter);
		};

		// avoid bouncing, and recogize the state changes (Push-Button will count hold seconds, based on this)
		m_timer_debounce = millis();
		return true;
	}
	return false;
}

// external button push
void button::interrupt(){
	m_interrupt_ready = true;
}
#endif
