#include <button.h>

button::button(){
	m_timer_button_down=0;
	m_counter=0;
};
button::~button(){
	logger.println(TOPIC_GENERIC_INFO, F("Button deleted"), COLOR_YELLOW);
};


bool button::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

uint8_t* button::get_key(){
	sprintf((char*)key,"B");
	return key;
}

void fooButton(){
	if(p_button){
		((button*)p_button)->interrupt();
	}
}

bool button::init(){
	logger.println(TOPIC_GENERIC_INFO, F("Button init"), COLOR_GREEN);
	// attache interrupt codepwm for button
	pinMode(BUTTON_INPUT_PIN, INPUT);
	digitalWrite(BUTTON_INPUT_PIN, HIGH); // pull up to avoid interrupts without sensor
	attachInterrupt(digitalPinToInterrupt(BUTTON_INPUT_PIN), fooButton, CHANGE);

	if (digitalRead(BUTTON_INPUT_PIN) == LOW) {
		wifiManager.startConfigPortal(CONFIG_SSID);
	}
}


bool button::loop(){
	// / see if we hold down the button for more then 6sec ///
	if (m_counter >= 10 && millis() - m_timer_button_down > BUTTON_TIMEOUT) {
		Serial.println(F("[SYS] Rebooting to setup mode"));
		delay(200);
		wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
		// ESP.reset(); // reboot and switch to setup mode right after that
	}
	return false; // i did nothing
}

uint8_t button::count_intervall_update(){
	return 0; // we have 0 value that we want to publish per minute
}

bool button::intervall_update(uint8_t slot){
	return false;
}

bool button::subscribe(){
	return true;
}

bool button::receive(uint8_t* p_topic, uint8_t* p_payload){
	return false; // not for me
}


bool button::publish(){
	if (m_state.get_outdated()) {
		boolean ret = false;

		logger.println(TOPIC_MQTT_PUBLISH, F("button push"), COLOR_GREEN);
		ret = client.publish(build_topic(MQTT_BUTTON_TOPIC,UNIT_TO_PC), "", true);
		if (ret) {
			m_state.outdated(false);
		}
		return ret;
	}
	return false;
}

// external button push
void button::interrupt(){
	// toggle, write to pin, publish to server
	if (digitalRead(BUTTON_INPUT_PIN) == LOW) {
		if (millis() - m_timer_button_down > BUTTON_DEBOUNCE) { // avoid bouncing
			// button down
			m_state.set(!m_state.get_value());
			// toggle status of both lights
			if(p_light){
				((light*)p_light)->toggle();
			}

			if (millis() - m_timer_button_down < BUTTON_TIMEOUT) {
				m_counter++;
			} else {
				m_counter = 1;
			}
			logger.print(TOPIC_BUTTON, F("push nr "));
			Serial.println(m_counter);
		};
		// Serial.print(".");
		m_timer_button_down = millis();
	}
	;
}
