#include "main.h"

uint8_t periodic_slot = 0;
uint32_t updateFastValuesTimer = 0;
uint32_t updateSlowValuesTimer = 0;
uint32_t timer_republish_avoid = 0;
uint32_t timer_connected_stop  = 0;
uint32_t timer_last_publish    = 0;
uint32_t m_animation_dimm_time = 0;

// prepare wifimanager variables
WiFiManagerParameter WiFiManager_mqtt_server_ip("mq_ip", "mqtt server ip", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_port("mq_port", "mqtt server port", "1883", 5);
WiFiManagerParameter WiFiManager_mqtt_capability("capability", "capability", "", 60);
WiFiManagerParameter WiFiManager_mqtt_client_short("sid", "mqtt short id", "devXX", 49);
WiFiManagerParameter WiFiManager_mqtt_server_login("login", "mqtt login", "", 15);
WiFiManagerParameter WiFiManager_mqtt_server_pw("pw", "mqtt pw", "", 15);

uint8_t active_p_pointer = 0;
uint8_t active_p_intervall_counter = 0;
capability ** active_p[MAX_CAPS];
capability ** active_intervall_p[MAX_CAPS];
char * p_trace;

// bool (*intervall_p[MAX_CAPS]);

// //////////////////////////////////////////////////////////////////////////////////////
// /////////////////////// helper to calc times a bit easier ///////////////////////////
bool relationship_timeout(float decelleration_factor, char * next_mode){
	uint16_t time_connected     = (timer_connected_stop - timer_connected_start) / 1000;
	uint16_t time_not_connected = (millis() - timer_connected_stop) / 1000;
	// min 45sec
	// max 20min
	// per second that we've been connected we increase the retry time by 0.2 sec
	uint16_t time_max_reconnect =
	  max(MIN_RECONNECT_TIME, min(MIN_RECONNECT_TIME + time_connected / CALC_RECONNECT_WEIGHT, MAX_RECONNECT_TIME));

	time_max_reconnect *= decelleration_factor;

	logger.addColor(COLOR_PURPLE);
	sprintf(m_msg_buffer, "Prev. connected for %i sec, disconnected for %i sec, total time before %s mode %i sec",
	  time_connected, time_not_connected, next_mode, time_max_reconnect);
	logger.pln(m_msg_buffer);
	logger.remColor(COLOR_PURPLE);

	if (time_not_connected > time_max_reconnect) {
		return true;
	}
	return false;
}

// /////////////////////////////////////////////////////////////////////////////////////
// /////////////////// function called when a MQTT message arrived /////////////////////
void callback(char * p_topic, byte * p_payload, uint16_t p_length){
	p_payload[p_length] = 0x00;
	logger.addColor(COLOR_PURPLE);
	logger.topic(TOPIC_MQTT_IN);

	if (p_length < MSG_BUFFER_SIZE - 10) { // limit
		sprintf(m_msg_buffer, "'%s' --> '%s'", p_topic, p_payload);
	} else {
		sprintf(m_msg_buffer, "'%s' --> ", p_topic);
		memcpy(m_msg_buffer+strlen(p_topic)+7, p_payload, MSG_BUFFER_SIZE-strlen(p_topic)-8);
		m_msg_buffer[MSG_BUFFER_SIZE-4]='.';
		m_msg_buffer[MSG_BUFFER_SIZE-3]='.';
		m_msg_buffer[MSG_BUFFER_SIZE-2]='.';
		m_msg_buffer[MSG_BUFFER_SIZE-1]=0x00;
	}
	logger.pln(m_msg_buffer);
	// Serial.printf("%s --> %s\r\n", p_topic, p_payload);
	logger.remColor(COLOR_PURPLE);

	// / will find the right component and execute the input
	for (uint8_t i = 0; active_p[i] != 0x00 && i < MAX_CAPS - 1; i++) {
		if ((*active_p[i])->receive((uint8_t *) p_topic, p_payload)) {
			return;
		}
	}
	// //////////////////////// SET LIGHT ON/OFF ////////////////////////
	// //////////////////////// SET LIGHT BRIGHTNESS AND COLOR ////////////////////////
	if (!strcmp(p_topic, build_topic(MQTT_SETUP_TOPIC, PC_TO_UNIT))) {
		// the setup topic is versital, Message is checked:
		// can be ON -> go to setup
		// can be http:// ... url for update
		// can be "reset" which will restart the unit
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // go to setup
			logger.println(TOPIC_MQTT, F("Go to setup"));
			delay(500);
			// TODO if (neo) { // restart Serial if neopixel are connected (they've reconfigured the RX pin/interrupt)
			Serial.end();
			delay(500);
			Serial.begin(115200);
			// }
			network.publish(build_topic(MQTT_SETUP_TOPIC, UNIT_TO_PC), (char *) "ok");
			wifiManager.startConfigPortal(CONFIG_SSID); // needs to be tested!
			// debug
			WiFi.printDiag(Serial);
		} else if (!strncmp_P((const char *) p_payload, "http", 4)) { // update
			if (network.m_connection_type == CONNECTION_DIRECT_CONNECTED) {
				logger.p(F("Update command with url found, trying to update from "));
				logger.pln((char *) p_payload);
				// have to create a copy, whenever we publish we'll override the current buffer
				char copy_buffer[sizeof(p_payload) / sizeof(p_payload[0])];
				strcpy(copy_buffer, (const char *) p_payload);
				network.publish(build_topic(MQTT_SETUP_TOPIC, UNIT_TO_PC), (char *) "ok");
				network.publish(build_topic("INFO", UNIT_TO_PC), (char *) "updating...");

				ESPhttpUpdate.rebootOnUpdate(false);
				HTTPUpdateResult res = ESPhttpUpdate.update(copy_buffer);
				if (res == HTTP_UPDATE_OK) {
					network.publish(build_topic("INFO", UNIT_TO_PC), (char *) "rebooting...");
					logger.pln(F("Update OK, rebooting"));
					ESP.restart();
				} else {
					network.publish(build_topic("INFO", UNIT_TO_PC), (char *) "update failed");
					logger.pln(F("Update failed"));
				}
			} else {
				network.publish(build_topic("INFO", UNIT_TO_PC), (char *) "Can't update. Mesh connection.");
				logger.p(F("Can't update. Mesh connection."));
			}
		} else if (!strcmp_P((const char *) p_payload, "reset")) { // reboot
			logger.p(F("Received reset command"));
			ESP.restart();
		} else if (!strncmp_P((const char *) p_payload, "setNW", 5)) { // set parameter
			// msg: set/SSID/PW/IP/CHK
			uint8_t slashes = 0;
			uint8_t chk     = 0;
			for (uint8_t i = 5; i < p_length; i++) {
				if (p_payload[i] == '/') { // counte slaeses
					slashes++;
				} else if (slashes <= 3) { // xor all chars before chk: SSID, PW, IP
					chk ^= p_payload[i];
				} else {
					// move it to a letter between A-Z
					while (chk < 'A') {
						chk += 'Z' - 'A';
					}
					while (chk > 'Z') {
						chk -= 'Z' - 'A';
					}
					if (p_payload[i] == chk && i == p_length - 1) {
						chk = 1;
					} else {
						sprintf(m_msg_buffer, "NW update failed, CKH wrong expected: %c", chk);
						logger.println(TOPIC_MQTT, m_msg_buffer, COLOR_RED);
						chk = 0;
						break;
					}
				}
			}
			if (chk == 1) {
				// SSID
				uint8_t end   = 6;
				uint8_t start = end;
				for (; p_payload[end] != '/'; end++) { }
				;
				memset(mqtt.nw_ssid, 0x00, sizeof(mqtt.nw_ssid));
				memcpy(mqtt.nw_ssid, ((const char *) p_payload) + start, end - start);
				// PW
				end++;
				start = end;
				for (; p_payload[end] != '/'; end++) { }
				;
				memset(mqtt.nw_pw, 0x00, sizeof(mqtt.nw_pw));
				memcpy(mqtt.nw_pw, ((const char *) p_payload) + start, end - start);
				// IP
				end++;
				start = end;
				for (; p_payload[end] != '/'; end++) { }
				;
				memset(mqtt.server_ip, 0x00, sizeof(mqtt.server_ip));
				memcpy(mqtt.server_ip, ((const char *) p_payload) + start, end - start);

				network.publish(build_topic(MQTT_SETUP_TOPIC, UNIT_TO_PC), (char *) "NW updated");
				logger.println(TOPIC_MQTT, F("NW updated"), COLOR_GREEN);
				delay(1000); // time for transmit before disconnect
				wifiManager.storeMqttStruct((char *) &mqtt, sizeof(mqtt));
				wifiManager.explainFullMqttStruct(&mqtt);
				WiFi.disconnect();
			} else {
				network.publish(build_topic(MQTT_SETUP_TOPIC, UNIT_TO_PC), (char *) "NW update failed");
				logger.println(TOPIC_MQTT, F("NW update failed"), COLOR_RED);
			};
		}
	}
	// //////////////////////// SET CAPABILITYS ////////////////////////
	else if (!strcmp(p_topic, build_topic(MQTT_CAPABILITY_TOPIC, PC_TO_UNIT))) {
		wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt)); // reload because the loadPheripherals procedure might have altered the info
		// logger.pln("compare:");
		// logger.pln((const char*)mqtt.cap);
		// logger.pln((const char*)p_payload);

		if (strcmp((const char *) mqtt.cap, (const char *) p_payload) != 0) {
			logger.print(TOPIC_MQTT, F("Capability update, new config:"));
			memcpy(mqtt.cap, p_payload, _min(sizeof(mqtt.cap), strlen((const char *) p_payload)));
			mqtt.cap[_min(sizeof(mqtt.cap), strlen((const char *) p_payload))] = 0x00; // ensure that at least there is room for 1 more comma
			logger.pln(mqtt.cap);
			// wifiManager.explainFullMqttStruct(&mqtt);
			wifiManager.storeMqttStruct((char *) &mqtt, sizeof(mqtt));
			// reload
			loadPheripherals((uint8_t *) mqtt.cap);
			logger.println(TOPIC_GENERIC_INFO, F("Disconnect MQTT to resubscribe"), COLOR_PURPLE);
			network.disconnectServer();
		}
	}
	// //////////////////// trace /////////////////////////////
	else if (!strcmp(p_topic, build_topic(MQTT_TRACE_TOPIC, PC_TO_UNIT))) {
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // switch on
			logger.enable_mqtt_trace(true);
			logger.println(TOPIC_MQTT, F("=== Trace activated ==="), COLOR_PURPLE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) { // switch off
			logger.enable_mqtt_trace(false);
		}
	}
	// //////////////////// OTA via MQTT /////////////////////////////
	else if (!strcmp(p_topic,
	   build_topic(MQTT_OTA_TOPIC, PC_TO_UNIT)) || !strcmp(p_topic, build_topic(MQTT_OTA_TOPIC, strlen(MQTT_OTA_TOPIC), PC_TO_UNIT, false))) {                   // dev specific and global
		// forward if global before processing, otherwise we reboot before we broadcast
		if (!strcmp(p_topic, build_topic(MQTT_OTA_TOPIC,strlen(MQTT_OTA_TOPIC), PC_TO_UNIT, false))) {
			network.broadcast_publish_down(p_topic, (char *) p_payload, p_length);
		}
		// now process
		network.mqtt_ota(p_payload, p_length);
	}
	// //////////////////// LOCK CONFIG /////////////////////////////
	else if (!strcmp(p_topic,
	   build_topic(MQTT_CONFIG_LOCK_TOPIC, PC_TO_UNIT))) {                   // dev specific 
		if (!strcmp_P((const char *) p_payload, STATE_ON)) { // switch on
			wifiManager._config_locked = true;
			logger.println(TOPIC_MQTT, F("config locked"), COLOR_PURPLE);
		} else if (!strcmp_P((const char *) p_payload, STATE_OFF)) { // switch off
			logger.println(TOPIC_MQTT, F("config unlocked"), COLOR_PURPLE);
			wifiManager._config_locked = false;
		}
	}
	// //////////////////// mesh /////////////////////////////
	else {
		// message is not for me, send it downhill to all clientSM
		network.broadcast_publish_down(p_topic, (char *) p_payload, p_length);
	}
} // callback

// /////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////// network function ///////////////////////////////////
void reconnect(){
	// Loop until we're reconnected
	if (timer_connected_stop < timer_connected_start) {
		timer_connected_stop = millis();
	}
	network.stopAP();

	// first check wifi
	WiFi.mode(WIFI_STA);                // avoid station and ap at the same time
	while (!network.connected(false)) { // this is wifi + mqtt/mesh connect
		wdt_reset();
		logger.println(TOPIC_MQTT, F("Currently not connected, checking wifi ..."), COLOR_RED);
		// check if we have valid credentials for a WiFi at all
		// the ssid and pw will be set to "" if the EEPROM was empty / invalid
		// if this is the case: start config Portal without waiting
		if ((strlen(mqtt.nw_ssid) == 0 && strlen(mqtt.nw_pw) == 0) ||
		(!strcmp(mqtt.nw_ssid,"new") && !strcmp(mqtt.nw_pw,"new"))) {
			wifiManager.startConfigPortal(CONFIG_SSID);
		}
		// each round, check wifi first
		if (WiFi.status() != WL_CONNECTED) {
			// if the ssid was programmed to be " ", connect to mesh
			if (strlen(mqtt.nw_ssid) == 1 && mqtt.nw_ssid[0] == ' ' &&
			  (network.getMeshMode() == MESH_MODE_FULL_MESH || network.getMeshMode() == MESH_MODE_CLIENT_ONLY))
			{
				logger.println(TOPIC_WIFI, F("No SSID trying mesh only"), COLOR_YELLOW);
				WiFi.mode(WIFI_STA); // setting this here again dramatically improved chances of connection
				network.MeshConnect();
			} else {
				WiFi.mode(WIFI_STA); // setting this here again dramatically improved chances of connection
				network.DirectConnect();
				// //////////////// MESH CONNECTION ///////////////////////
				// try mesh if wifi directly did not work after some time
				// this will be true after (min 45*0.8=35 sec)
				// or when the ssid is not programmed, set SSID to
#ifdef WITH_MESH
				if (network.getMeshMode() == MESH_MODE_FULL_MESH || network.getMeshMode() == MESH_MODE_CLIENT_ONLY) {
					if (relationship_timeout(0.8, (char *) "MESH")) {
						logger.println(TOPIC_WIFI, F("Can't connect directly fast enough, trying mesh in addition"), COLOR_YELLOW);
						network.MeshConnect();
					}
				}
#endif
			}
			// //////////////// MESH CONNECTION ///////////////////////
		} else {
			logger.println(TOPIC_WIFI, F("online"), COLOR_GREEN);
		}
		// only try mqtt after wifi is estabilshed
		if (WiFi.status() == WL_CONNECTED) {
			// Attempt to connect
			logger.print(TOPIC_MQTT, F("connecting with id: "));
			logger.pln(mqtt.dev_short);
			// if (c l i e n t . connect(mqtt.dev_short, mqtt.login, mqtt.pw)) {
			if (network.connectServer(mqtt.dev_short, mqtt.login, mqtt.pw)) {
				logger.println(TOPIC_MQTT, F("connected"), COLOR_GREEN);

				network.loopCheck();
				network.publishRouting();

				// capability error checking, this has to be before the subscription. Because once a capability input is received
				// we have to reload the eeprom information and therefore loose the info about which caps were consumed
				uint8_t p_fill=0;
				for(uint8_t i=0; mqtt.cap[i]; i++){
					if(mqtt.cap[i]!='x'){
						if(mqtt.cap[i]==',' && (p_fill==0 || mqtt.cap[p_fill-1]==',')){ // only move meaningful comma
							continue;
						}
						mqtt.cap[p_fill] = mqtt.cap[i];
						p_fill++;
					}
				}
				if(p_fill>1){ // at least one letter + ','
					mqtt.cap[p_fill-1]=0x00; // remove last ','
					snprintf_P((char*)m_msg_buffer, MSG_BUFFER_SIZE, PSTR("Unsatisfied config: %s"), (char*)mqtt.cap);
					logger.print(TOPIC_MQTT_PUBLISH, build_topic("error", UNIT_TO_PC), COLOR_RED);
					logger.p((char *) " -> ");
					logger.pln(m_msg_buffer);
				} else {
					m_msg_buffer[0]=0x00; // clear previously reported retained errors
				}
				network.publish(build_topic("error", UNIT_TO_PC), m_msg_buffer);
				wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt)); // reload because we've altered the info and we need the original content


				// ... and resubscribe
				network.subscribe(build_topic(MQTT_TRACE_TOPIC, PC_TO_UNIT)); // MQTT_TRACE_TOPIC topic
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_TRACE_TOPIC, PC_TO_UNIT), COLOR_GREEN);

				network.subscribe(build_topic(MQTT_SETUP_TOPIC, PC_TO_UNIT)); // setup topic
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_SETUP_TOPIC, PC_TO_UNIT), COLOR_GREEN);

				network.subscribe(build_topic(MQTT_CAPABILITY_TOPIC, PC_TO_UNIT)); // capability topic
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_CAPABILITY_TOPIC, PC_TO_UNIT), COLOR_GREEN);

				network.subscribe(build_topic(MQTT_OTA_TOPIC, PC_TO_UNIT)); // MQTT_OTA_TOPIC topic
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_OTA_TOPIC, PC_TO_UNIT), COLOR_GREEN);

				network.subscribe(build_topic(MQTT_OTA_TOPIC, strlen(MQTT_OTA_TOPIC), PC_TO_UNIT, false)); // MQTT_OTA_TOPIC but global topic
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_OTA_TOPIC, strlen(MQTT_OTA_TOPIC), PC_TO_UNIT, false), COLOR_GREEN);

				network.subscribe(build_topic(MQTT_CONFIG_LOCK_TOPIC, PC_TO_UNIT)); // lock config 
				logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_CONFIG_LOCK_TOPIC, PC_TO_UNIT), COLOR_GREEN);


				for (uint8_t i = 0; active_p[i] != 0x00 && i < MAX_CAPS - 1; i++) {
					// Serial.printf("subscibe e%i\r\n",i);
					// delay(500);
					(*active_p[i])->subscribe();
				}
				logger.println(TOPIC_MQTT, F("subscribing finished"));

				// INFO publishing
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%s", VERSION);
				network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("INFO", UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln(m_msg_buffer);


				// WIFI publishing
				network.publish(build_topic("SSID", UNIT_TO_PC), (char *) WiFi.SSID().c_str());
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("SSID", UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln((char *) (WiFi.SSID().c_str()));

				// BSSID publishing
				uint8_t * bssid = WiFi.BSSID();
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[5], bssid[4], bssid[3], bssid[2],
				  bssid[1],
				  bssid[0]);
				network.publish(build_topic("BSSID", UNIT_TO_PC), m_msg_buffer);
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("BSSID", UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln(m_msg_buffer);

				// MAC publishing
				uint8_t mac[6];
				WiFi.macAddress(mac);
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
				  mac[4],
				  mac[5]);
				network.publish(build_topic("MAC", UNIT_TO_PC), m_msg_buffer);
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("MAC", UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln(m_msg_buffer);

				// IP publishing
				IPAddress ip = WiFi.localIP();
				snprintf(m_msg_buffer, MSG_BUFFER_SIZE, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
				network.publish(build_topic("IP", UNIT_TO_PC), m_msg_buffer);
				logger.print(TOPIC_MQTT_PUBLISH, build_topic("IP", UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln(m_msg_buffer);

				// CAP publishing
				network.publish(build_topic(MQTT_CAPABILITY_TOPIC, UNIT_TO_PC), mqtt.cap);
				logger.print(TOPIC_MQTT_PUBLISH, build_topic(MQTT_CAPABILITY_TOPIC, UNIT_TO_PC), COLOR_GREEN);
				logger.p((char *) " -> ");
				logger.pln(mqtt.cap);

				logger.println(TOPIC_MQTT, F("publishing finished"));

				if (network.getMeshMode() == MESH_MODE_FULL_MESH || network.getMeshMode() == MESH_MODE_HOST_ONLY) {
					network.startAP();
				}
				timer_connected_start = millis();
			} // if MQTT network.connect ok
		}  // wifi status connected

		// if client is still not connected: wait
		if (!network.connected()) {
			// connect failed
			// min 45 sec, per 5 sec connected add one sec, max 1200 sec
			if (relationship_timeout(1, (char *) "AP")) {
				// time to start the AP
				logger.pln(F("Can't connect, starting AP"));
				// restart Serial if neopixel/audio are connected (they've reconfigured the RX pin/interrupt)
				/*if (p_neo || p_play) {
					Serial.end();
					delay(500);
					Serial.begin(115200);
				}*/
				// reload mqtt data
				wifiManager.loadMqttStruct((char *) &mqtt, sizeof(mqtt)); // reload because we've altered the info and we need the original content
				// run wifi manager
				wifiManager.startConfigPortal(CONFIG_SSID);
				// reinit capability if needed
#ifdef WITH_NEO
				if(p_neo){
					p_neo->init();
				}
#endif
#ifdef WITH_PLAY				
				if(p_play){
					p_play->init();
				}
#endif				
				// resets timer
				timer_connected_stop  = millis();
				timer_connected_start = millis();
				logger.pln(F("Config AP closed"));
				// debug
				WiFi.printDiag(Serial);
			} else { // not yet time to access point, wait 5 sec
				logger.println(TOPIC_WIFI, F("connect failed, trying again in 5 seconds "));
				// Serial.printf("%i/%i\r\n", tries, max_tries);

				// only wait if the MQTT broker was not available,
				// no need to wait if the wifi was the reason, that will take longer anyway
				if (WiFi.status() == WL_CONNECTED) {
					// Wait 5 seconds before retrying
					delay(5000);
				}
			}
		} // end of "if (!network.connected()) {"
		  // after this .. start over
	} // while (!network.connected()) {
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
	((light *) p_light)->toggle();
}

// save config to eeprom
void saveConfigCallback(){
	sprintf(mqtt.server_ip, "%s", WiFiManager_mqtt_server_ip.getValue());
	sprintf(mqtt.login, "%s", WiFiManager_mqtt_server_login.getValue());
	sprintf(mqtt.pw, "%s", WiFiManager_mqtt_server_pw.getValue());
	sprintf(mqtt.server_port, "%s", WiFiManager_mqtt_server_port.getValue());
	sprintf(mqtt.dev_short, "%s", WiFiManager_mqtt_client_short.getValue());
	sprintf(mqtt.cap, "%s", WiFiManager_mqtt_capability.getValue());
	sprintf(mqtt.nw_ssid, "%s", wifiManager._ssid.c_str());
	sprintf(mqtt.nw_pw, "%s", wifiManager._pass.c_str());
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
	}
	logger.pln(F("=== Loaded parameters: ==="));
	wifiManager.explainFullMqttStruct(&mqtt);
	mqtt.cap[sizeof(mqtt.cap) - 2] = 0x00; // sure to make sure he string is determine
	loadPheripherals((uint8_t *) mqtt.cap);
	logger.pln(F("=== End of parameters ==="));
} // loadConfig

void loadPheripherals(uint8_t * config){
	// erase
	for (uint8_t i = 0; i < MAX_CAPS; i++) {
		if (active_p[i]) {
			delete *active_p[i];
			active_p[i] = 0x00;
		}
	}
	// hanky: activate button just to see if we should go to setup
#ifdef WITH_BUTTON
	logger.println(TOPIC_GENERIC_INFO, F("checking direct setup"), COLOR_PURPLE);
	p_button = new button();
	p_button->init();
	delete p_button;
	p_button = 0x00;
#endif

	// create objects for real
	// activate
	active_p_pointer = 0;
	active_p_intervall_counter = 0;
	logger.println(TOPIC_GENERIC_INFO, F("activating capabilities"), COLOR_PURPLE);
	// logger.p("RAM before creating objects ");
	// logger.pln(system_get_free_heap_size());
#ifdef WITH_ADC
	bake(new ADC(), &p_adc, config);
#endif
#ifdef WITH_BUTTON
	bake(new button(), &p_button, config);
	if(p_button!=NULL){
		wifiManager.setButtonCallback(WifiButton);
	}
#endif
#ifdef WITH_SL
	bake(new simple_light(), &p_simple_light, config);
#endif
#ifdef WITH_RSL
	bake(new remote_simple_light(), &p_remote_simple_light, config);
#endif
#ifdef WITH_RSSI
	bake(new rssi(), &p_rssi, config);
#endif
#ifdef WITH_PWM
	bake(new PWM(((uint8_t *) "PWM"), 4, 5, 16, 0, 0), &p_pwm, config);     // SONOFF PWM TODO: THIS IS LIKELY NOT WORKING AS EXPECTED
	bake(new PWM(((uint8_t *) "PW2"), 4, 4, 4, 0, 0), &p_pwm2, config);     // kolja 2
	bake(new PWM(((uint8_t *) "PW3"), 15, 13, 12, 14, 4), &p_pwm3, config); // H801 module 5 mosfets on gpio: R,G,B,W1,W2
#endif
#ifdef WITH_SHELLY_DIMMER
	bake(new shelly_dimmer(),&p_shellyDimmer, config);  // shelly dimmer 
#endif
#ifdef WITH_PIR
	bake(new PIR(), &p_pir, config);                 // Kolja_v2
#endif
#ifdef WITH_SERIALSERVER
	bake(new SerialServer(), &p_SerSer, config);
#endif
#ifdef WITH_DHT22
	bake(new J_DHT22(), &p_dht, config);
#endif
#ifdef WITH_DS
	bake(new J_DS(), &p_ds, config);
#endif
#ifdef WITH_AI
	bake(new AI(), &p_ai, config);
#endif
#ifdef WITH_BONE
	bake(new BOne(), &p_bOne, config);
#endif
#ifdef WITH_NEOSTRIP
	bake(new NeoStrip(), &p_neo, config);
#endif
#ifdef WITH_HLW
	bake(new J_hlw8012(), &p_hlw, config);
#endif
#ifdef WITH_NL
	bake(new night_light(), &p_nl, config);
#endif
#ifdef WITH_TUYA_BRIDGE
	bake(new tuya_bridge(), &p_rfb, config);
#endif
#ifdef WITH_GPIO
	bake(new J_GPIO(), &p_gpio, config);
#endif
	//bake(new husqvarna(), &p_husqvarna, config);
	bake(new no_mesh(), &p_no_mesh, config);
#ifdef WITH_UPTIME
	bake(new uptime(), &p_uptime, config);
#endif
#ifdef WITH_FREQ
	bake(new freq(), &p_freq, config);
#endif
#ifdef WITH_PLAY
	bake(new play(), &p_play, config);
#endif
#ifdef WITH_REC
	bake(new record(), &p_rec, config);
#endif
#ifdef WITH_EM
	bake(new energy_meter(), &p_em, config);
#endif
#ifdef WITH_EM_BIN
	bake(new energy_meter_bin(), &p_em_bin, config);
#endif
#ifdef WITH_IR
	bake(new ir(), &p_ir, config);
#endif
#ifdef WITH_ADS1015
	bake(new ads1015(), &p_ads1015, config);
#endif
#ifdef WITH_HX711
	bake(new hx711(), &p_hx711, config);
#endif
#ifdef WITH_CRASH
	bake(new crash(), &p_crash, config);
#endif
#ifdef WITH_COUNTER
	bake(new counter(), &p_count, config);
#endif
#ifdef WITH_EBUS
	bake(new ebus(), &p_ebus, config);
#endif
#ifdef WITH_FIREPLACE
	bake(new fireplace(), &p_fireplace, config);
#endif
#ifdef WITH_AUTARCO
	bake(new autarco(), &p_autarco, config);
#endif
	bake(new light(), &p_light, config);


	// logger.p("RAM after init objects ");
	// logger.pln(system_get_free_heap_size());

	logger.println(TOPIC_GENERIC_INFO, F("linking capabilities"), COLOR_PURPLE);

	for(uint8_t i=0; i<active_p_pointer; i++){
		//Serial.printf("check if component %s has a dep\r\n",(*active_p[i])->get_key());
		// check of all objects if they had a dependency
		if(strlen((char*)((*active_p[i])->get_dep()))>0){
			//Serial.printf("yes: %s\r\n",(*active_p[i])->get_dep());
			// if so, find the dependency and tell them that we're depending on them
			for(uint8_t ii=0; ii<active_p_pointer; ii++){
				//Serial.printf("compare to key %s\r\n",(*active_p[ii])->get_key());
	 			if(!strcmp((char*)((*active_p[i])->get_dep()), (char*)((*active_p[ii])->get_key()))){
					//Serial.println("register");
	 				// so everyone and their sister need, get_key and reg_provider?
	 				(*active_p[ii])->reg_provider(*active_p[i],(*active_p[i])->get_key());
	 			}
	 		}
 		}
	}

	// try to sort entries in active_intervall_p more evenly
	// so we know how many slots we have in total ... active_p_intervall_counter 
	// and we have the list of components in active_p and the amount in active_p_pointer lets try to resort
	for(uint8_t i=0; i<active_p_intervall_counter; i++){
		active_intervall_p[i]=0x00;
	}
	
	for(uint8_t i=0; i<active_p_pointer; i++){
		if(((*active_p[i])->count_intervall_update())>0){
			float interval = (float)active_p_intervall_counter / (float)((*active_p[i])->count_intervall_update());
			uint8_t start;
			for (start=0; start<active_p_intervall_counter; start++){
				if(active_intervall_p[start]==0x00){
					break;
				}
			}
			for(uint8_t ii=0;ii<((*active_p[i])->count_intervall_update());ii++){
				bool found = false;
				for(uint8_t pos = start + round(ii*interval); pos<active_p_intervall_counter; pos++){
					if(active_intervall_p[pos]==0x00){
						active_intervall_p[pos] = active_p[i];
						//sprintf(m_msg_buffer,"comp %i, inter %i, pos %i",i,ii,pos);
						//logger.pln(m_msg_buffer);
						found = true;
						break;
					}
				}
				if(!found){
					for(uint8_t pos = 0; pos<active_p_intervall_counter; pos++){
						if(active_intervall_p[pos]==0x00){
							active_intervall_p[pos] = active_p[i];
							found = true;
							break;
						}
					}	
				}
				if(!found){
					sprintf(m_msg_buffer,"issue with comp %i, inter %i, no pos found",i,ii);
					logger.pln(m_msg_buffer);
				}
			}
		}
	}


	logger.println(TOPIC_GENERIC_INFO, F("capabilities loaded"), COLOR_PURPLE);
	sprintf(m_msg_buffer,"%i intervall updates",active_p_intervall_counter);
	logger.println(TOPIC_GENERIC_INFO, m_msg_buffer, COLOR_PURPLE);
} // loadPheripherals

/* build topics with device id on the fly
 *
 * usually call version below e.g. build_topic(MQTT_ENERGY_METER_TOTAL_TOPIC, UNIT_TO_PC)
 * 
 * if MQTT_ENERGY_METER_TOTAL_TOPIC has parameter, e.g "ads_a%i" you can use this and call it
 * build_topic((MQTT_ENERGY_METER_TOTAL_TOPIC, 7, UNIT_TO_PC, true, 1)
 * where 7 is the max length of ads_a%i with the %i filled (+0x00 at the end) and 1 is the actual parameter
 * 
 */
char * build_topic(const char * topic, uint8_t max_pre_topic_length, uint8_t pc_shall_R_or_S, bool with_dev, ...){
	// create string
	char* pre_topic = new char[max_pre_topic_length];
	va_list args;
	va_start (args,with_dev);
	vsprintf(pre_topic, topic, args);
	va_end (args);
	
	if (with_dev && strlen(mqtt.dev_short)+strlen(pre_topic)+4 < sizeof(m_topic_buffer)){
		sprintf(m_topic_buffer, "%s/%c/%s", mqtt.dev_short, pc_shall_R_or_S, pre_topic);
		delete pre_topic;
		return m_topic_buffer;
	} else if (!with_dev && strlen(mqtt.dev_short)+strlen(pre_topic)+1 < sizeof(m_topic_buffer)){
		sprintf(m_topic_buffer, "%c/%s", pc_shall_R_or_S, pre_topic);
		delete pre_topic;
		return m_topic_buffer;
	} else {
		// topic too long.
		snprintf_P((char*)m_msg_buffer, MSG_BUFFER_SIZE, PSTR("Topic too long: "), pre_topic);
		memcpy(m_msg_buffer+16,topic,min(strlen(pre_topic),(unsigned int)10));
		memcpy(m_msg_buffer+16+min(strlen(pre_topic),(unsigned int)10),"...",3);
		m_msg_buffer[16+min(strlen(pre_topic),(unsigned int)10)+4]=0x00;
		delete pre_topic;
		logger.print(TOPIC_MQTT_PUBLISH, build_topic("error", UNIT_TO_PC), COLOR_RED);
		logger.p((char *) " -> ");
		logger.pln(m_msg_buffer);
		network.publish(build_topic("error", UNIT_TO_PC), m_msg_buffer);

	}	
	
	return m_topic_buffer;
}


char * build_topic(const char * topic, uint8_t pc_shall_R_or_S){
	return build_topic(topic, strlen(topic), pc_shall_R_or_S, true); 
}


// //////////////////////////////// network function ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////

// /////////////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////// SETUP ///////////////////////////////////
void setup(){
	Serial.begin(115200);
	logger.init();
	for (uint8_t i = 0; i < 10; i++) {
		logger.p(i);
		logger.p(F(".. "));
	//	delay(500);
	}
	wdt_enable(WDTO_8S); // start watchdog

	logger.pln(F(""));
	logger.pln(F(""));
	logger.pln(F("========== INFO ========== "));
	logger.p(F("Startup v"));
	logger.pln(F(VERSION));
	logger.pln(F("+ Flash:"));
	if (ESP.getFlashChipRealSize() != ESP.getFlashChipSize()) {
		if (ESP.getFlashChipRealSize() > ESP.getFlashChipSize()) {
			logger.p(F("  warning"));
		} else {
			logger.p(F("  CRITICAL"));
		}
		logger.pln(F(", wrong flash config"));
		sprintf_P(m_msg_buffer, (const char *) F("  Dev has %i but is configured with size %i"),
		  ESP.getFlashChipRealSize(), ESP.getFlashChipSize());
		logger.pln(m_msg_buffer);
	} else {
		logger.pln(F("  Flash config correct"));
	}
	logger.pln(F("+ RAM:"));
	sprintf_P(m_msg_buffer, (const char *) F("  available  %i"), system_get_free_heap_size());
	logger.pln(m_msg_buffer);
	logger.pln(F("========== INFO ========== "));
	// /// init the serial and print debug /////

	// /// init the led /////
	for (uint8_t i = 0; i < MAX_CAPS; i++) {
		active_intervall_p[i] = 0x00;
	}

	// //// start wifi manager
	wifiManager.setAPCallback(configModeCallback);
	wifiManager.setLightToggleCallback(toggleCallback);
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setConfigPortalTimeout(MAX_AP_TIME);
	wifiManager.setConnectTimeout(MAX_CON_TIME);
	wifiManager.setMqtt(&mqtt); // save to reuse structure (only to save memory)
	wifiManager.setCapability(active_p); // save to have a link to all capabilities

	// init the MQTT connection
	randomSeed(millis());

	// load all paramters!
	loadConfig();
	// get reset reason
	if (p_light) {
		if (ESP.getResetInfoPtr()->reason == REASON_DEFAULT_RST) {
			// set some light on regular power up
			logger.println(TOPIC_GENERIC_INFO, F("PowerUp. Set all lights on"), COLOR_PURPLE);
			((light *) p_light)->setColor(255, 255, 255);
			((light *) p_light)->setState(true);
		} else {
			logger.pln(F(""));
			logger.println(TOPIC_GENERIC_INFO, F("WatchDog Reset. Set all lights off"), COLOR_PURPLE);
			((light *) p_light)->setState(false);
		}
	}

	logger.pln(F("+ RAM:"));
	sprintf_P(m_msg_buffer, (const char *) F("  available  %i"), system_get_free_heap_size());
	logger.pln(m_msg_buffer);
	logger.pln(F("=== End of Setup ==="));
	logger.pln(F(" "));

	timer_connected_start = 0;
} // setup

// /////////////////////////////////////////// SETUP ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////

// /////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////// LOOP ///////////////////////////////////
bool uninterrupted;
void loop(){
	wdt_reset();
	// handle network .. like parse and react on incoming data
	network.receive_loop();

	if (!network.connected()) {
		reconnect();
		// reconnect will NEVER leave with not connected network, it will actaully loop until a connection is established
	}


	// // dimming end ////
	uninterrupted = false;
	for (uint8_t i = 0; i < active_p_pointer && active_p[i] != 0x00; i++) {
		if ((*active_p[i])->loop()) { // uninterrupted loop request ... don't execute others
			uninterrupted = true;
			wdt_reset();
			break;
		}
	}


	// // send periodic updates ////
	if (!uninterrupted) {
		if (millis() - timer_last_publish > PUBLISH_TIME_OFFSET) {
			for (uint8_t i = 0; i < active_p_pointer && active_p[i] != 0x00; i++) {
				wdt_reset();
				if ((*active_p[i])->publish()) { // some one published something urgend, stop others
					timer_republish_avoid = millis();
					timer_last_publish    = millis();
					break;
				}
			}
		}

		// // send periodic updates ////
		if (active_p_intervall_counter > 0) {
			if (millis() - updateFastValuesTimer > (60000UL / active_p_intervall_counter) && millis() - timer_last_publish >
			  PUBLISH_TIME_OFFSET) {
				updateFastValuesTimer = millis();
				// make sure that every entrie receives its own 0,1,2,3 slots
				uint8_t user_slot = 0;
				for(uint8_t i=0; i<periodic_slot; i++){
					if(active_intervall_p[periodic_slot] == active_intervall_p[i]){
						user_slot++;
					}
				}
				wdt_reset();
				(*active_intervall_p[periodic_slot])->intervall_update(user_slot);
				periodic_slot = (periodic_slot + 1) % active_p_intervall_counter;
			}
		}

		// // trace ////
		if (millis() - timer_last_publish > PUBLISH_TIME_OFFSET) {
			p_trace = (char *) logger.loop();
			if (p_trace) {
				// Serial.printf("total %i, end %i,%i,%i\r\n",strlen(t),t[strlen(t)-1],t[strlen(t)-2],t[strlen(t)-3]);
				// if(t[strlen(t)-2]==13 && t[strlen(t)-1]==10){
				//	t[strlen(t)-2]=0x00;
				// }
				wdt_reset();
				network.publish(build_topic(MQTT_TRACE_TOPIC, UNIT_TO_PC), p_trace);
				timer_last_publish = millis();
			}
		}
	}
} // loop5

// //////////////////////////////////////////// LOOP ///////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////
bool bake(capability * p_obj, capability ** p_handle, uint8_t * config){
	if (p_obj->parse(config)) { // true = obj active
		*p_handle = p_obj;
		(*p_handle)->init();
		// store object in active capability list
		active_p[active_p_pointer] = p_handle;
		if (active_p_pointer < MAX_CAPS - 1) {
			active_p_pointer++;
		}
		// get the amount of intervall updates and store a pointer to the object
		for (uint8_t ii = 0; ii < p_obj->count_intervall_update(); ii++) {
			active_intervall_p[active_p_intervall_counter] = p_handle;
			if (active_p_intervall_counter < MAX_CAPS - 1) {
				active_p_intervall_counter++;
			}
		}
		return true;
	} else {
		delete p_obj;
		*p_handle = 0x00;
	}
	return false;
}
// /////////////////////////////////////////////////////////////////////////////////////
char* str_rpl(char* in, char old, char replacement){
	return str_rpl(in,old,replacement,strlen(in));
}

char* str_rpl(char* in, char old, char replacement, uint8_t len){
	for(uint16 i=0; i<len; i++){
		if(in[i]==old){
			in[i]=replacement;
		}
	}
	return in;
}
// /////////////////////////////////////////////////////////////////////////////////////
char* discovery_topic_bake(const char* topic,...){
	// create string
	va_list args;
	va_start (args,topic);
	char* t = new char[strlen(topic)+strlen(mqtt.dev_short)];
	vsprintf(t, topic, args);
	va_end (args);

	//Serial.print("Topic input ");
	//Serial.println(t);

	char* pos = strstr(t,mqtt.dev_short);
	if(pos!=NULL){
		str_rpl(pos, '/', '_',strlen(mqtt.dev_short));  // remove all '/' and replace them with '_'
	}

	//Serial.print("Topic output ");
	//Serial.println(t);

	return t; // don't forget to free this as well
}
// /////////////////////////////////////////////////////////////////////////////////////
char* discovery_topic_bake_2(const char* domain, const char* topic){
	//static constexpr char MQTT_DISCOVERY_AUTARCO_TOTAL_KWH_TOPIC[] = "homeassistant/sensor/%s_autarco_kwh/config";

	char* t = new char[14+strlen(domain)+1+strlen(mqtt.dev_short)+1+strlen(topic)+8];
	sprintf(t,"homeassistant/%s/%s_%s/config",domain,mqtt.dev_short,topic);
	//Serial.print("Topic input ");
	//Serial.println(t);

	char* pos = strstr(t,mqtt.dev_short);
	if(pos!=NULL){
		str_rpl(pos, '/', '_',strlen(mqtt.dev_short));  // remove all '/' and replace them with '_'
	}

	//Serial.print("Topic output ");
	//Serial.println(t);

	return t; // don't forget to free this as well
}
// /////////////////////////////////////////////////////////////////////////////////////
char* discovery_message_bake_2(const char* domain, const char* topic, const char* unit){
	if(strcmp(unit,UNIT_NONE)!=0){
		char* t = new char[65+3*strlen(mqtt.dev_short)+3*strlen(topic)+strlen(unit)];
		sprintf(t,"{\"name\":\"%s_%s\", \"stat_t\": \"%s/r/%s\", \"unit_of_meas\": \"%s\", \"uniq_id\":\"%s_%s\"}",mqtt.dev_short,topic,mqtt.dev_short,topic,unit,mqtt.dev_short,topic);
		return t;
	}
	char* t = new char[45+3*strlen(mqtt.dev_short)+3*strlen(topic)];
	sprintf(t,"{\"name\":\"%s_%s\", \"stat_t\": \"%s/r/%s\", \"uniq_id\":\"%s_%s\"}",mqtt.dev_short,topic,mqtt.dev_short,topic,mqtt.dev_short,topic);
	return t;
}

// /////////////////////////////////////////////////////////////////////////////////////

bool discovery(const char* domain, const char* topic, const char* unit){
	char* t = discovery_topic_bake_2(domain,topic);
	char* m = discovery_message_bake_2(domain,topic,unit);
	bool ret = network.publish(t,m);
	delete[] m;
	delete[] t;
	return ret;
}

// /////////////////////////////////////////////////////////////////////////////////////