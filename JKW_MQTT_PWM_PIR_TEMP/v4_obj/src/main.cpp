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
WiFiManagerParameter WiFiManager_mqtt_capability("capability", "capability", "", 60);
WiFiManagerParameter WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 6);
WiFiManagerParameter WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);

const uint8_t hb[34] = { 27, 27, 27, 27, 27, 27, 17, 27, 37, 21, 27, 27, 27, 27, 27, 52, 71,
	                        91, 99, 91, 33,  0, 12, 29, 52, 33, 21, 26, 33, 26, 20, 27, 27, 27 };

// buffer used to send/receive data with MQTT, can not be done with the m_topic_buffer, as both as need simultaniously


uint8_t active_p_pointer=0;
uint8_t active_p_intervall_counter=0;
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
	if (!strcmp(p_topic, build_topic(MQTT_SETUP_TOPIC,PC_TO_UNIT))) {
		// the setup topic is versital, Message is checked:
		// can be ON -> go to setup
		// can be http:// ... url for update
		// can be "reset" which will restart the unit
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // go to setup
			logger.println(TOPIC_MQTT, F("Go to setup"));
			delay(500);
			//TODO if (neo) { // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
				Serial.end();
				delay(500);
				Serial.begin(115200);
			//}
			client.publish(build_topic(MQTT_SETUP_TOPIC,UNIT_TO_PC), "ok", true);
			wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
			// debug
			WiFi.printDiag(Serial);
		} else if (!strncmp_P((const char *) p_payload, "http", 4)) { // update
			logger.p(F("Update command with url found, trying to update from "));
			logger.pln((char*)p_payload);
			// have to create a copy, whenever we publish we'll override the current buffer
			char copy_buffer[sizeof(p_payload)/sizeof(p_payload[0])];
			strcpy(copy_buffer,(const char*)p_payload);
			client.publish(build_topic(MQTT_SETUP_TOPIC,UNIT_TO_PC), "ok", true);
			client.publish(build_topic("INFO",UNIT_TO_PC), "updating...", true);

			ESPhttpUpdate.rebootOnUpdate(false);
			HTTPUpdateResult res = ESPhttpUpdate.update(copy_buffer);
			if (res == HTTP_UPDATE_OK) {
				client.publish(build_topic("INFO",UNIT_TO_PC), "rebooting...", true);
				logger.pln(F("Update OK, rebooting"));
				ESP.restart();
			} else {
				client.publish(build_topic("INFO",UNIT_TO_PC), "update failed", true);
				logger.pln(F("Update failed"));
			}
		} else if (!strcmp_P((const char *) p_payload, "reset")) { // reboot
			logger.p(F("Received reset command"));
			ESP.restart();
		} else if (!strncmp_P((const char *) p_payload, "set", 3)) { // set parameter
				if (!strncmp_P( ((const char *)p_payload)+3, "S", 1)) { // set SSID
					strcpy(mqtt.nw_ssid,((const char *)p_payload)+4);
				} else if (!strncmp_P( ((const char *)p_payload)+3, "P", 1)) { // set SSID
					strcpy(mqtt.nw_pw,((const char *)p_payload)+4);
					WiFi.disconnect();
				};
				wifiManager.storeMqttStruct((char *) &mqtt, sizeof(mqtt));
				wifiManager.explainFullMqttStruct(&mqtt);
		}
	}
	// //////////////////////// SET CAPABILITYS ////////////////////////
	else if (!strcmp(p_topic, build_topic(MQTT_CAPABILITY_TOPIC,PC_TO_UNIT))) {
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
			// reload
			loadPheripherals((uint8_t*)mqtt.cap);
			logger.println(TOPIC_GENERIC_INFO, F("Disconnect MQTT to resubscribe"),COLOR_PURPLE);
			client.disconnect();
		}
	}
	////////////////////// trace /////////////////////////////
	else if(!strcmp(p_topic, build_topic(MQTT_TRACE_TOPIC,PC_TO_UNIT))){
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
			wifiManager.connectWifi(mqtt.nw_ssid, mqtt.nw_pw);
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
				client.subscribe(build_topic(MQTT_TRACE_TOPIC,PC_TO_UNIT)); // MQTT_TRACE_TOPIC topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_TRACE_TOPIC,PC_TO_UNIT), COLOR_GREEN);

				client.subscribe(build_topic(MQTT_SETUP_TOPIC,PC_TO_UNIT)); // setup topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_SETUP_TOPIC,PC_TO_UNIT), COLOR_GREEN);

				client.subscribe(build_topic(MQTT_CAPABILITY_TOPIC,PC_TO_UNIT)); // capability topic
				client.loop();
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_CAPABILITY_TOPIC,PC_TO_UNIT), COLOR_GREEN);

				for(uint8_t i = 0; active_p[i]!=0x00 ; i++){
					//Serial.printf("subscibe e%i\r\n",i);
					//delay(500);
					(*active_p[i])->subscribe();
				}
				logger.println(TOPIC_MQTT, F("subscribing finished"));

				// INFO publishing
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%s %s", PINOUT, VERSION);
				client.publish(build_topic("INFO",UNIT_TO_PC), m_msg_buffer, true);
				client.loop();
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("INFO",UNIT_TO_PC), COLOR_GREEN);
				logger.p((char*)" -> ");
				logger.pln(m_msg_buffer);


				// WIFI publishing
				client.publish(build_topic("SSID",UNIT_TO_PC), WiFi.SSID().c_str(), true);
				client.loop();
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("SSID",UNIT_TO_PC), COLOR_GREEN);
				logger.p((char*)" -> ");
				logger.pln((char*)(WiFi.SSID().c_str()));

				// BSSID publishing
				uint8_t * bssid = WiFi.BSSID();
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[5], bssid[4], bssid[3], bssid[2],
				  bssid[1],
				  bssid[0]);
				client.publish(build_topic("BSSID",UNIT_TO_PC), m_msg_buffer, true);
				client.loop();
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("BSSID",UNIT_TO_PC), COLOR_GREEN);
				logger.p((char*)" -> ");
				logger.pln(m_msg_buffer);

				// CAP publishing
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%s %s", PINOUT, VERSION);
				client.publish(build_topic(MQTT_CAPABILITY_TOPIC,UNIT_TO_PC), mqtt.cap, true);
				client.loop();
				logger.print(TOPIC_MQTT_PUBLISH, build_topic(MQTT_CAPABILITY_TOPIC,UNIT_TO_PC), COLOR_GREEN);
				logger.p((char*)" -> ");
				logger.pln(mqtt.cap);

				logger.println(TOPIC_MQTT, F("publishing finished"));

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
	wifiManager.resetParameter();
	wifiManager.addParameter(&WiFiManager_mqtt_server_port);
	wifiManager.addParameter(&WiFiManager_mqtt_capability);
	wifiManager.addParameter(&WiFiManager_mqtt_server_ip);
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
	sprintf(mqtt.cap, "%s", WiFiManager_mqtt_capability.getValue());
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
		logger.pln(F("Config load passed"));
		// set identifier for SSID and menu
		wifiManager.setCustomIdElement(mqtt.dev_short);
		// resuract eeprom values instead of defaults
		WiFiManager_mqtt_capability.setValue(mqtt.cap);
		WiFiManager_mqtt_server_ip.setValue(mqtt.server_ip);
		WiFiManager_mqtt_server_port.setValue(mqtt.server_port);
		WiFiManager_mqtt_client_short.setValue(mqtt.dev_short);
		WiFiManager_mqtt_server_login.setValue(mqtt.login);
		WiFiManager_mqtt_server_pw.setValue(mqtt.pw);
	} else {
		wifiManager.setCustomIdElement("");
		logger.pln(F("Config load failed"));
		sprintf(mqtt.dev_short,"new");
	}
	logger.pln(F("=== Loaded parameters: ==="));
	wifiManager.explainFullMqttStruct(&mqtt);
	mqtt.cap[sizeof(mqtt.cap)-2]=0x00; // sure to make sure he string is determine
	loadPheripherals((uint8_t*)mqtt.cap);
	logger.pln(F("=== End of parameters ==="));
} // loadConfig

void loadPheripherals(uint8_t* config){
	// erase
	for(uint8_t i = 0; i<MAX_PERIPHERALS ; i++){
		if(active_p[i]){
			delete *active_p[i];
			active_p[i] = 0x00;
		}
	}
	// activate
	active_p_pointer=0;
	active_p_intervall_counter = 0;
	logger.println(TOPIC_GENERIC_INFO, F("activating peripherals"), COLOR_PURPLE);

	//logger.p("RAM before creating objects ");
	//logger.pln(system_get_free_heap_size());
	// create objects
	bake(new ADC(), &p_adc, config);
	bake(new button(), &p_button, config);
	bake(new simple_light(), &p_simple_light, config);
	bake(new rssi(), &p_rssi, config);
	bake(new PWM(((uint8_t*)"PWM"),4,5,16,0,0), &p_pwm, config); // SONOFF PWM TODO: THIS IS LIKELY NOT WORKING AS EXPECTED
	bake(new PWM(((uint8_t*)"PW2"),4,4,4,0,0), &p_pwm2, config); // kolja 2
	bake(new PWM(((uint8_t*)"PW3"),15,13,12,14,4), &p_pwm3, config); // H801 module 5 mosfets on gpio: R,G,B,W1,W2
	bake(new PIR(((uint8_t*)"PIR"),14), &p_pir, config); // SONOFF PIR
	bake(new PIR(((uint8_t*)"PI2"),5), &p_pir2, config); // Kolja_v2
	bake(new J_DHT22(), &p_dht, config);
	bake(new J_DS(), &p_ds, config);
	bake(new AI(), &p_ai, config);
	bake(new BOne(), &p_bOne, config);
	bake(new NeoStrip(), &p_neo, config);
	bake(new light(), &p_light, config);
	bake(new J_hlw8012(), &p_hlw, config);

	//logger.p("RAM after init objects ");
	//logger.pln(system_get_free_heap_size());

	logger.println(TOPIC_GENERIC_INFO, F("linking peripherals"), COLOR_PURPLE);

	// make this more generic .... like ...
	// go through all entities, check if they have a dependency if so
	// call the dependency with this opbject
	/*
	for(uint8_t i=0; i<active_p_pointer; i++){
		// check of all objects if they had a dependency
		if(strlen(active_p[i]->get_dep())>0){
			// if so, find the dependency and tell them that we're depending on them
			for(uint8_t ii=0; ii<active_p_pointer; ii++){
				if(strcmp(active_p[i]->get_dep(), active_p[ii]->get_key())){
					// so everyone and their sister need, get_key and reg_provider?
					((light*)active_p[ii])->reg_provider(active_p[i],active_p[i]->get_key()));
				}
			}
		}
	}
	*/
	// set one provider
	if(p_simple_light){
		((light*)p_light)->reg_provider(p_simple_light,T_SL);
	} else if(p_pwm){
		((light*)p_light)->reg_provider(p_pwm,T_PWM);
	} else if(p_pwm2){
		((light*)p_light)->reg_provider(p_pwm2,T_PWM);
	} else if(p_pwm3){
		((light*)p_light)->reg_provider(p_pwm3,T_PWM);
	} else if(p_neo){
		((light*)p_light)->reg_provider(p_neo,T_NEO);
	} else if(p_bOne){
		((light*)p_light)->reg_provider(p_bOne,T_BOne);
	} else if(p_ai){
		((light*)p_light)->reg_provider(p_ai,T_AI);
	}

	logger.println(TOPIC_GENERIC_INFO, F("peripherals loaded"), COLOR_PURPLE);
	wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt)); // reload because the loadPheripherals procedure might have altered the info
}

// build topics with device id on the fly
char * build_topic(const char * topic, uint8_t pc_shall_R_or_S){
	if(pc_shall_R_or_S!=PC_TO_UNIT && pc_shall_R_or_S!=UNIT_TO_PC){
		pc_shall_R_or_S=PC_TO_UNIT;
	}
	sprintf(m_topic_buffer, "%s/%c/%s", mqtt.dev_short, pc_shall_R_or_S, topic);
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
		sprintf_P(m_msg_buffer, (const char*)F("  Dev has %i but is configured with size %i"), ESP.getFlashChipRealSize(), ESP.getFlashChipSize());
		logger.pln(m_msg_buffer);
	} else {
		logger.pln(F("  Flash config correct"));
	}
	logger.pln(F("+ RAM:"));
	sprintf_P(m_msg_buffer, (const char*)F("  available  %i"), system_get_free_heap_size());
	logger.pln(m_msg_buffer);
	logger.pln(F("========== INFO ========== "));
	// /// init the serial and print debug /////

	// /// init the led /////
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
	logger.pln(F("=== End of Setup ==="));
	logger.pln(F(" "));
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
				// make sure that every entrie receives its own 0,1,2,3 slots
				uint8_t user_slot=0;
				if(periodic_slot>0){
					for(user_slot=periodic_slot-1; user_slot>=0; user_slot--){
						if(active_intervall_p[periodic_slot]!=active_intervall_p[user_slot]){
							/* assuming	[0] = pir, [1] = adc, [2] = dht, [3] = dht
							frist dht call: periodic_slot = 2, user slot = 1, adc != dht --> 2-1-1 = 0
							second dht call: periodic_slot = 3, user slot = 2, dht == dht, user_slot = 1, adc != dht --> 3-1-1 = 1
							*/
							user_slot= periodic_slot-user_slot-1;
							break;
						}
					}
				}
				(*active_intervall_p[periodic_slot])->intervall_update(user_slot);
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
				client.publish(build_topic(MQTT_TRACE_TOPIC,UNIT_TO_PC), p_trace);
				timer_last_publish    = millis();
			}
		}
	}

} // loop

// //////////////////////////////////////////// LOOP ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////
bool bake(peripheral* p_obj,peripheral** p_handle, uint8_t* config){
	if(p_obj->parse(config)){ // true = obj active
		*p_handle = p_obj;
		(*p_handle)->init();
		// store object in active peripheral list
		active_p[active_p_pointer]=p_handle;
		active_p_pointer++;
		// get the amount of intervall updates and store a pointer to the object
		for(uint8_t ii = 0; ii<p_obj->count_intervall_update(); ii++){
			active_intervall_p[active_p_intervall_counter] = p_handle;
			active_p_intervall_counter++;
		}
		return true;
	} else {
		delete p_obj;
		*p_handle = 0x00;
	}
	return false;
}
