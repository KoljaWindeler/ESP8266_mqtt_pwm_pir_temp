#include "main.h"

uint8_t  periodic_slot         = 0;
uint32_t updateFastValuesTimer = 0;
uint32_t updateSlowValuesTimer = 0;
uint32_t timer_republish_avoid = 0;
uint32_t timer_connected_start = 0;
uint32_t timer_connected_stop  = 0;
uint32_t timer_last_publish    = 0;
uint32_t m_animation_dimm_time = 0;

// prepare wifimanager variables
WiFiManagerParameter WiFiManager_mqtt_server_ip("mq_ip", "mqtt server ip", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_port("mq_port", "mqtt server port", "1883", 5);
WiFiManagerParameter WiFiManager_mqtt_capability_b0("cap_0", "PWM Leds", 0, 2, true);       // length must be at least tw0 .. why? // Capability Bit0 = PWM LEDs connected
WiFiManagerParameter WiFiManager_mqtt_capability_b1("cap_1", "Neopixel", 0, 2, true);       // Bit1 = Neopixel,
WiFiManagerParameter WiFiManager_mqtt_capability_b2("cap_2", "Avoid relay", 0, 2, true);    // Bit2 = Avoid relay
WiFiManagerParameter WiFiManager_mqtt_capability_b3("cap_3", "Sonoff B1", 0, 2, true);      // Bit3 = sonoff b1
WiFiManagerParameter WiFiManager_mqtt_capability_b4("cap_4", "AiTinker light", 0, 2, true); // Bit4 = aitinker
WiFiManagerParameter WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 6);
WiFiManagerParameter WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);

const uint8_t hb[34] = { 27, 27, 27, 27, 27, 27, 17, 27, 37, 21, 27, 27, 27, 27, 27, 52, 71,
	                        91, 99, 91, 33,  0, 12, 29, 52, 33, 21, 26, 33, 26, 20, 27, 27, 27 };

// buffer used to send/receive data with MQTT, can not be done with the m_topic_buffer, as both as need simultaniously


uint8_t active_p_pointer=0;
uint8_t active_p_intervall_counter=0;
peripheral **all_p[MAX_PERIPHERALS];
peripheral **active_p[MAX_PERIPHERALS];
peripheral **active_intervall_p[MAX_PERIPHERALS];
char* p_trace;

//bool (*intervall_p[MAX_PERIPHERALS]);



// /////////////////////////////////////////////////////////////////////////////////////
// /////////////////// function called when a MQTT message arrived /////////////////////
void callback(char * p_topic, byte * p_payload, unsigned int p_length){
	p_payload[p_length] = 0x00;
	logger.addColor(COLOR_PURPLE);
	logger.topic(TOPIC_MQTT_IN);
	sprintf(m_msg_buffer,"%s --> %s\r\n", p_topic, p_payload);
	logger.p(m_msg_buffer);
	//Serial.printf("%s --> %s\r\n", p_topic, p_payload);
	logger.remColor(COLOR_PURPLE);

	/// will find the right component and execute the input
	for(uint8_t i = 0; active_p[i]!=0x00 ; i++){
		if((*active_p[i])->receive((uint8_t*)p_topic, p_payload)){
			return;
		}
	}
	// //////////////////////// SET LIGHT ON/OFF ////////////////////////
	// //////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
	if (!strcmp(p_topic, build_topic(MQTT_SETUP_TOPIC))) {
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // go to setup
			logger.println(TOPIC_MQTT, F("Go to setup"));
			delay(500);
			//TODO if (neo) { // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
				Serial.end();
				delay(500);
				Serial.begin(115200);
			//}
			client.publish(build_topic(MQTT_SETUP_TOPIC), "ok", true);
			wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
			// debug
			WiFi.printDiag(Serial);
		} else if (!strncmp_P((const char *) p_payload, "http", 4)) { // update
			logger.p(F("Update command with url found, trying to update from "));
			logger.pln((char*)p_payload);
			// have to create a copy, whenever we publish we'll override the current buffer
			char copy_buffer[sizeof(p_payload)/sizeof(p_payload[0])];
			strcpy(copy_buffer,(const char*)p_payload);
			client.publish(build_topic(MQTT_SETUP_TOPIC), "ok", true);
			client.publish(build_topic("/INFO"), "updating...", true);

			ESPhttpUpdate.rebootOnUpdate(false);
			HTTPUpdateResult res = ESPhttpUpdate.update(copy_buffer);
			if (res == HTTP_UPDATE_OK) {
				client.publish(build_topic("/INFO"), "rebooting...", true);
				logger.pln(F("Update OK, rebooting"));
				ESP.restart();
			} else {
				client.publish(build_topic("/INFO"), "update failed", true);
				logger.pln(F("Update failed"));
			}
		} else if (!strcmp_P((const char *) p_payload, "reset")) { // reboot
			logger.p(F("Received reset command"));
			ESP.restart();
		}
	}
	// //////////////////////// SET CAPABILITYS ////////////////////////
	else if (!strcmp(p_topic, build_topic(MQTT_CAPABILITY_TOPIC))) {
		wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt)); // reload because the loadPheripherals procedure might have altered the info
		//logger.pln("compare:");
		//logger.pln((const char*)mqtt.cap);
		//logger.pln((const char*)p_payload);

		if(strcmp((const char*)mqtt.cap ,(const char*)p_payload)!=0){
			logger.print(TOPIC_MQTT, F("Capability update, new config:"));
			memcpy(mqtt.cap,p_payload,_min(sizeof(mqtt.cap),strlen((const char*)p_payload)));
			mqtt.cap[_min(sizeof(mqtt.cap),strlen((const char*)p_payload))]=0x00; // ensure that at least there is room for 1 more comma
			logger.pln(mqtt.cap);
			//wifiManager.explainFullMqttStruct(&mqtt);
			wifiManager.storeMqttStruct((char *) &mqtt, sizeof(mqtt));
			// unload
			for(uint8_t i = 0; i<active_p_pointer && active_p[i]!=0x00 ; i++){
				delete (*active_p[i]);
				*active_p[i]=0x00;
				active_p[i]=0x00;
			}
			// reload
			loadPheripherals((uint8_t*)mqtt.cap);
			logger.println(TOPIC_GENERIC_INFO, F("Disconnect MQTT to resubscribe"),COLOR_PURPLE);
			client.disconnect();
		}
	}
	////////////////////// trace /////////////////////////////
	else if(!strcmp(p_topic, build_topic(MQTT_TRACE_COMMAND_TOPIC))){
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // switch on
			logger.set_active(true);
			logger.println(TOPIC_MQTT, F("=== Trace activated ==="),COLOR_PURPLE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) { // switch off
			logger.set_active(false);
		}
	}
} // callback

// /////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////// network function ///////////////////////////////////
void reconnect(){
	// Loop until we're reconnected
	if (timer_connected_stop < timer_connected_start) {
		timer_connected_stop = millis();
	}
	logger.pln(F("\r\n"));

	// first check wifi
	WiFi.mode(WIFI_STA); // avoid station and ap at the same time
	while (!client.connected()) {
		logger.println(TOPIC_MQTT, F("Currently not connected, checking wifi ..."), COLOR_RED);
		// each round, check wifi first
		if (WiFi.status() != WL_CONNECTED) {
			logger.println(TOPIC_WIFI, F("Currently not connected, initiate new connection ..."), COLOR_RED);
			// mqtt.nw_ssid, mqtt.nw_pw or autoconnect?
			wifiManager.connectWifi("", "");
		} else {
			logger.println(TOPIC_WIFI, F("online"), COLOR_GREEN);
		}
		// only try mqtt after wifi is estabilshed
		if (WiFi.status() == WL_CONNECTED) {
			// Attempt to connect
			logger.print(TOPIC_MQTT, F("connecting with id: "));
			logger.pln(mqtt.dev_short);
			if (client.connect(mqtt.dev_short, mqtt.login, mqtt.pw)) {
				logger.println(TOPIC_MQTT, F("connected"), COLOR_GREEN);

				// ... and resubscribe
				client.subscribe(build_topic(MQTT_TRACE_COMMAND_TOPIC)); // MQTT_TRACE_COMMAND_TOPIC topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_TRACE_COMMAND_TOPIC), COLOR_GREEN);

				client.subscribe(build_topic(MQTT_SETUP_TOPIC)); // setup topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_SETUP_TOPIC), COLOR_GREEN);

				client.subscribe(build_topic(MQTT_CAPABILITY_TOPIC)); // capability topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_CAPABILITY_TOPIC), COLOR_GREEN);

				for(uint8_t i = 0; active_p[i]!=0x00 ; i++){
					//Serial.printf("subscibe e%i\r\n",i);
					(*active_p[i])->subscribe();
				}

				// INFO publishing
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%s %s", PINOUT, VERSION);
				client.publish(build_topic("/INFO"), m_msg_buffer, true);
				client.loop();

				// WIFI publishing
				client.publish(build_topic("/SSID"), WiFi.SSID().c_str(), true);
				client.loop();

				// BSSID publishing
				uint8_t * bssid = WiFi.BSSID();
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[5], bssid[4], bssid[3], bssid[2],
				  bssid[1],
				  bssid[0]);
				client.publish(build_topic("/BSSID"), m_msg_buffer, true);
				client.loop();

				logger.println(TOPIC_MQTT, F("subscribing finished"));

				timer_connected_start = millis();
			} // if MQTT client.connect ok
		}  // wifi status connected

		// if client is still not connected: wait
		if (!client.connected()) {
			// connect failed
			// min 45 sec, per 5 sec connected add one sec, max 1200 sec
			uint16_t time_connected     = (timer_connected_stop - timer_connected_start) / 1000;
			uint16_t time_not_connected = (millis() - timer_connected_stop) / 1000;
			uint16_t time_max_reconnect =
			  _max(MIN_RECONNECT_TIME, _min(MIN_RECONNECT_TIME + time_connected / CALC_RECONNECT_WEIGHT, MAX_RECONNECT_TIME));

			logger.addColor(COLOR_PURPLE);
			sprintf(m_msg_buffer,
			  "MQTT was previously connected %i sec\r\nMQTT is disconnected for %i sec\r\nMax time before starting AP mode %i sec\r\n",
			  time_connected, time_not_connected, time_max_reconnect);
			logger.p(m_msg_buffer);
			logger.remColor(COLOR_PURPLE);

			if (time_not_connected > time_max_reconnect) {
				// time to start the AP
				logger.pln(F("Can't connect, starting AP"));
				if(p_neo) { // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
					Serial.end();
					delay(500);
					Serial.begin(115200);
				}
				wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
				timer_connected_stop  = millis();           // resets timer
				timer_connected_start = millis();
				logger.pln(F("Config AP closed"));
				// debug
				WiFi.printDiag(Serial);
			}
			// not yet time to access point, wait 5 sec
			else {
				logger.print(TOPIC_WIFI, F("connect failed, "));
				logger.p(client.state());
				logger.pln(F(", try again in 5 seconds "));
				// Serial.printf("%i/%i\r\n", tries, max_tries);

				// only wait if the MQTT broker was not available,
				// no need to wait if the wifi was the reason, that will take longer anyway
				if (WiFi.status() == WL_CONNECTED) {
					// Wait 5 seconds before retrying
					delay(5000);
				}
			}
		} // end of "if (!client.connected()) {"
		  // after this .. start over
	} // while (!client.connected()) {
}  // reconnect

void configModeCallback(WiFiManager * myWiFiManager){
	wifiManager.addParameter(&WiFiManager_mqtt_server_ip);
	wifiManager.addParameter(&WiFiManager_mqtt_server_port);
	wifiManager.addParameter(&WiFiManager_mqtt_capability_b0);
	wifiManager.addParameter(&WiFiManager_mqtt_capability_b1);
	wifiManager.addParameter(&WiFiManager_mqtt_capability_b2);
	wifiManager.addParameter(&WiFiManager_mqtt_capability_b3);
	wifiManager.addParameter(&WiFiManager_mqtt_capability_b4);
	wifiManager.addParameter(&WiFiManager_mqtt_client_short);
	wifiManager.addParameter(&WiFiManager_mqtt_server_login);
	wifiManager.addParameter(&WiFiManager_mqtt_server_pw);
	// prepare wifimanager variables
	wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 255), IPAddress(255, 255, 255, 0));
	logger.pln(F("Entered config mode"));
}

// this is a callback so we can toggle the lights via the wifimanager and identify the light
void toggleCallback(){
	((light*)p_light)->toggle();
}

// save config to eeprom
void saveConfigCallback(){
	sprintf(mqtt.server_ip, "%s", WiFiManager_mqtt_server_ip.getValue());
	sprintf(mqtt.login, "%s", WiFiManager_mqtt_server_login.getValue());
	sprintf(mqtt.pw, "%s", WiFiManager_mqtt_server_pw.getValue());
	sprintf(mqtt.server_port, "%s", WiFiManager_mqtt_server_port.getValue());
	sprintf(mqtt.dev_short, "%s", WiFiManager_mqtt_client_short.getValue());
	// collect capability
	mqtt.cap[0] = 0x00; // reset to start clean, add bits, shift to ascii
	if (WiFiManager_mqtt_capability_b0.getValue()[0] == '1') {
		mqtt.cap[0] |= RGB_PWM_BITMASK;
	}
	if (WiFiManager_mqtt_capability_b1.getValue()[0] == '1') {
		mqtt.cap[0] |= NEOPIXEL_BITMASK;
	}
	if (WiFiManager_mqtt_capability_b2.getValue()[0] == '1') {
		mqtt.cap[0] |= AVOID_RELAY_BITMASK;
	}
	if (WiFiManager_mqtt_capability_b3.getValue()[0] == '1') {
		mqtt.cap[0] = SONOFF_B1_BITMASK; // hard set
	}
	if (WiFiManager_mqtt_capability_b4.getValue()[0] == '1') {
		mqtt.cap[0] = AITINKER_BITMASK; // hard set
	}
	mqtt.cap[0] += '0';

	logger.pln(F("=== Saving parameters: ==="));
	wifiManager.explainFullMqttStruct(&mqtt);
	logger.pln(F("=== End of parameters ==="));
	wifiManager.storeMqttStruct((char *) &mqtt, sizeof(mqtt));
	logger.pln(F("Configuration saved, restarting"));
	delay(2000);
	ESP.reset(); // eigentlich muss das gehen so, .. // we can't change from AP mode to client mode, thus: reboot
} // saveConfigCallback

void loadConfig(){
	// fill the mqtt element with all the data from eeprom
	if (wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt))) {
		// set identifier for SSID and menu
		wifiManager.setCustomIdElement(mqtt.dev_short);
		// resuract eeprom values instead of defaults
		WiFiManager_mqtt_server_ip.setValue(mqtt.server_ip);
		WiFiManager_mqtt_server_port.setValue(mqtt.server_port);
		WiFiManager_mqtt_client_short.setValue(mqtt.dev_short);
		WiFiManager_mqtt_server_login.setValue(mqtt.login);
		WiFiManager_mqtt_server_pw.setValue(mqtt.pw);
		// technically wrong, as the value will be 1,2,4,8,... and not 0/1
		// but still works as the test is value!=0
		sprintf(m_msg_buffer, "%i", (((uint8_t) (mqtt.cap[0]) - '0') & RGB_PWM_BITMASK));
		WiFiManager_mqtt_capability_b0.setValue(m_msg_buffer);
		sprintf(m_msg_buffer, "%i", (((uint8_t) (mqtt.cap[0]) - '0') & NEOPIXEL_BITMASK));
		WiFiManager_mqtt_capability_b1.setValue(m_msg_buffer);
		sprintf(m_msg_buffer, "%i", (((uint8_t) (mqtt.cap[0]) - '0') & AVOID_RELAY_BITMASK));
		WiFiManager_mqtt_capability_b2.setValue(m_msg_buffer);
		sprintf(m_msg_buffer, "%i", (((uint8_t) (mqtt.cap[0]) - '0') & SONOFF_B1_BITMASK));
		WiFiManager_mqtt_capability_b3.setValue(m_msg_buffer);
		sprintf(m_msg_buffer, "%i", (((uint8_t) (mqtt.cap[0]) - '0') & AITINKER_BITMASK));
		WiFiManager_mqtt_capability_b4.setValue(m_msg_buffer);
	} else {
		wifiManager.setCustomIdElement("");
	}
	logger.pln(F("=== Loaded parameters: ==="));
	wifiManager.explainFullMqttStruct(&mqtt);
	mqtt.cap[sizeof(mqtt.cap)-2]=0x00; // sure to make sure he string is determine
	loadPheripherals((uint8_t*)mqtt.cap);
	logger.pln(F("=== End of parameters ==="));
} // loadConfig

void loadPheripherals(uint8_t* peripherals){
	// capabilities
	logger.println(TOPIC_GENERIC_INFO, F("creating peripherals"), COLOR_PURPLE);
	// erase
	for(uint8_t i = 0; i<MAX_PERIPHERALS ; i++){
		if(all_p[i]){
			delete *all_p[i];
			all_p[i] = 0x00;
			active_p[i] = 0x00;
		}
	}
	active_p_pointer=0;

	//logger.p("RAM before creating objects ");
	//logger.pln(system_get_free_heap_size());
	// create objects
	p_adc = new ADC();
	p_button = new button();
	p_pir = new PIR();
	p_simple_light = new simple_light();
	p_rssi = new rssi();
	p_pwm = new PWM();
	p_dht = new J_DHT22();
	p_ds = new J_DS();
	p_ai = new AI();
	p_bOne = new BOne();
	p_neo = new NeoStrip();
	p_light = new light();

	//logger.p("RAM after creating objects ");
	//logger.pln(system_get_free_heap_size());

	// register
	all_p[active_p_pointer]=&p_pwm;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_adc;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_button;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_pir;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_simple_light;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_rssi;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_dht;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_ds;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_ai;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_bOne;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_neo;
	active_p_pointer++;
	all_p[active_p_pointer]=&p_light;
	active_p_pointer++;
	// activate
	active_p_pointer=0;
	active_p_intervall_counter = 0;
	// remove me
	//uint8_t p_string[40];
	//sprintf((char*)p_string,"ADC,SL,PIR,PWM,R,B,DHT,DS,LIG,NEO");
	//sprintf((char*)p_string,"ADC,PIR,R,B,DHT,DS,LIG,NEO");
	logger.println(TOPIC_GENERIC_INFO, F("activating peripherals"), COLOR_PURPLE);

	for(uint8_t i = 0; all_p[i]!=0x00 ; i++){
		if((*all_p[i])->parse(peripherals)){ // true = obj active
			(*all_p[i])->init();
			// store object in active peripheral list
			active_p[active_p_pointer]=all_p[i];
			active_p_pointer++;
			// get the amount of intervall updates and store a pointer to the object
			for(uint8_t ii = 0; ii<(*all_p[i])->count_intervall_update(); ii++){
				active_intervall_p[active_p_intervall_counter] = &(*(all_p[i]));
				active_p_intervall_counter++;
			}
		} else {
			delete (*all_p[i]);
			*all_p[i]=0x00;
			all_p[i]=0x00;
		}
	}

	//logger.p("RAM after init objects ");
	//logger.pln(system_get_free_heap_size());

	logger.println(TOPIC_GENERIC_INFO, F("linking peripherals"), COLOR_PURPLE);
	// set one provider
	if(p_simple_light){
		((light*)p_light)->reg_provider(p_simple_light,T_SL);
	} else if(p_pwm){
		((light*)p_light)->reg_provider(p_pwm,T_PWM);
	} else if(p_neo){
		((light*)p_light)->reg_provider(p_neo,T_NEO);
	} else if(p_bOne){
		((light*)p_light)->reg_provider(p_bOne,T_BOne);
	} else if(p_ai){
		((light*)p_light)->reg_provider(p_ai,T_AI);
	}

	logger.println(TOPIC_GENERIC_INFO, F("peripherals loaded"), COLOR_PURPLE);
}

// build topics with device id on the fly
char * build_topic(const char * topic){
	sprintf(m_topic_buffer, "%s%s", mqtt.dev_short, topic);
	return m_topic_buffer;
}

// //////////////////////////////// network function ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////

// /////////////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////// SETUP ///////////////////////////////////
void setup(){
	// /// init the serial and print debug /////
	Serial.begin(115200);
	logger.init();
	for (uint8_t i = 0; i < 10; i++) {
		logger.p(i);
		logger.p(F(".. "));
		delay(500);
	}
	logger.pln(F(""));
	logger.pln(F(""));
	logger.pln(F("========== INFO ========== "));
	logger.p(F("Startup v"));
	logger.pln(F(VERSION));
	logger.p(F("Pinout "));
	logger.pln(F(PINOUT));
	logger.pln(F("+ Flash:"));
	if (ESP.getFlashChipRealSize() != ESP.getFlashChipSize()) {
		if (ESP.getFlashChipRealSize() > ESP.getFlashChipSize()) {
			logger.p(F("  warning"));
		} else {
			logger.p(F("  CRITICAL"));
		}
		logger.pln(F(", wrong flash config"));
		logger.p(F("  Dev has "));
		logger.p(ESP.getFlashChipRealSize());
		logger.p(F(" but is configured with size "));
		logger.pln(ESP.getFlashChipSize());
	} else {
		logger.pln(F("  Flash config correct"));
	}
	logger.pln(F("+ RAM:"));
	logger.p(F("  available "));
	logger.pln(system_get_free_heap_size());
	logger.pln(F("========== INFO ========== "));
	// /// init the serial and print debug /////

	// /// init the led /////
	for(uint8_t i = 0; i<MAX_PERIPHERALS; i++){
		all_p[i]=0x00;
	}
	for(uint8_t i = 0; i<MAX_PERIPHERALS; i++){
		active_intervall_p[i]=0x00;
	}
	// load all paramters!
	loadConfig();
	// get reset reason
	if(p_light){
		if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST) {
			// set some light on regular power up
			logger.println(TOPIC_GENERIC_INFO, F("PowerUp. Set all lights on"), COLOR_PURPLE);
			((light*)p_light)->setColor(255,255,255);
			((light*)p_light)->setState(true);
		} else {
			logger.pln(F(""));
			logger.println(TOPIC_GENERIC_INFO, F("WatchDog Reset. Set all lights off"), COLOR_PURPLE);
			((light*)p_light)->setState(false);
		}
	}

	// //// start wifi manager
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setLightToggleCallback(toggleCallback);
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setConfigPortalTimeout(MAX_AP_TIME);
	wifiManager.setConnectTimeout(MAX_CON_TIME);
	wifiManager.setMqtt(&mqtt); // save to reuse structure (only to save memory)

	// init the MQTT connection
	client.setServer(mqtt.server_ip, atoi(mqtt.server_port));
	client.setCallback(callback);
	randomSeed(millis());
} // setup

// /////////////////////////////////////////// SETUP ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////

// /////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////// LOOP ///////////////////////////////////
bool uninterrupted;
void loop(){
	if (!client.connected()) {
		reconnect();
	}
	client.loop();
	// // dimming end ////
	uninterrupted = false;
	for(uint8_t i = 0; i<active_p_pointer && active_p[i]!=0x00 ; i++){
		if((*active_p[i])->loop()){ // uninterrupted loop request ... don't execute others
			uninterrupted=true;
			break;
		}
	}

	// // send periodic updates ////
	if(!uninterrupted){
		if (millis() - timer_last_publish > PUBLISH_TIME_OFFSET) {
			for(uint8_t i = 0; i<active_p_pointer && active_p[i]!=0x00 ; i++){
				if((*active_p[i])->publish()){ // some one published something urgend, stop others
					timer_republish_avoid = millis();
					timer_last_publish    = millis();
					break;
				}
			}
		}

		// // send periodic updates ////
		if(active_p_intervall_counter>0){
			if (millis() - updateFastValuesTimer > (60000UL/active_p_intervall_counter) && millis() - timer_last_publish > PUBLISH_TIME_OFFSET) {
				updateFastValuesTimer = millis();
				(*active_intervall_p[periodic_slot])->intervall_update(periodic_slot);
				periodic_slot = (periodic_slot + 1) % active_p_intervall_counter;
			}
		}

	// // trace ////
		if (millis() - timer_last_publish > PUBLISH_TIME_OFFSET) {
			p_trace=(char *)logger.loop();
			if(p_trace){
				//Serial.printf("total %i, end %i,%i,%i\r\n",strlen(t),t[strlen(t)-1],t[strlen(t)-2],t[strlen(t)-3]);
				//if(t[strlen(t)-2]==13 && t[strlen(t)-1]==10){
				//	t[strlen(t)-2]=0x00;
				//}
				client.publish(build_topic(MQTT_TRACE_STATUS_TOPIC), p_trace);
				timer_last_publish    = millis();
			}
		}
	}

} // loop

// //////////////////////////////////////////// LOOP ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////
