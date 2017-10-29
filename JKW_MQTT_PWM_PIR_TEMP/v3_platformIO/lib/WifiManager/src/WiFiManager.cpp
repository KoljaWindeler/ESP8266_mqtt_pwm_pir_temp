/**************************************************************
*  WiFiManager is a library for the ESP8266/Arduino platform
*  (https://github.com/esp8266/Arduino) to enable easy
*  configuration and reconfiguration of WiFi credentials using a Captive Portal
*  inspired by:
*  http://www.esp8266.com/viewtopic.php?f=29&t=2520
*  https://github.com/chriscook8/esp-arduino-apboot
*  https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
*  Built by AlexT https://github.com/tzapu
*  Licensed under MIT license
**************************************************************/

#include "WiFiManager.h"
// #include "v2_rgb.h"

WiFiManagerParameter::WiFiManagerParameter(const char * custom){
	_id = NULL;
	_placeholder = NULL;
	_length      = 0;
	_value       = NULL;

	_customHTML = custom;
}

WiFiManagerParameter::WiFiManagerParameter(const char * id, const char * placeholder, const char * defaultValue,
  int length){
	init(id, placeholder, defaultValue, length, "", false);
}

WiFiManagerParameter::WiFiManagerParameter(const char * id, const char * placeholder, const char * defaultValue,
  int length,
  bool isBoolean){
	init(id, placeholder, defaultValue, length, "", isBoolean);
}

WiFiManagerParameter::WiFiManagerParameter(const char * id, const char * placeholder, const char * defaultValue,
  int length,
  const char * custom){
	init(id, placeholder, defaultValue, length, custom, false);
}

bool WiFiManagerParameter::getType(){
	return _isBoolean;
}

void WiFiManagerParameter::init(const char * id, const char * placeholder, const char * defaultValue, int length,
  const char * custom,
  bool isBoolean){
	_id = id;
	_placeholder = placeholder;
	_length      = length;
	_value       = new char[length + 1];
	_isBoolean   = isBoolean;
	for (int i = 0; i < length; i++) {
		_value[i] = 0;
	}
	if (defaultValue != NULL) {
		strncpy(_value, defaultValue, length);
		// Serial.println("setting value: ");
		// Serial.println(_value);
	}

	_customHTML = custom;
}

const char * WiFiManagerParameter::getValue(){
	return _value;
}

void WiFiManagerParameter::setValue(char * value){
	strncpy(_value, value, _length);
	// Serial.println("setting value2: ");
	// Serial.println(_value);
}

const char * WiFiManagerParameter::getID(){
	return _id;
}

const char * WiFiManagerParameter::getPlaceholder(){
	return _placeholder;
}

int WiFiManagerParameter::getValueLength(){
	return _length;
}

const char * WiFiManagerParameter::getCustomHTML(){
	return _customHTML;
}

WiFiManager::WiFiManager(){
	_customIdElement = "";
}

void WiFiManager::addParameter(WiFiManagerParameter * p){
	_params[_paramsCount] = p;
	_paramsCount++;
	Serial.print(F("Adding parameter: "));
	Serial.print(p->getID());
	Serial.print(F(" -> "));
	Serial.println(p->getValue());
}

void WiFiManager::setupConfigPortal(){
	dnsServer.reset(new DNSServer());
	server.reset(new ESP8266WebServer(80));

	DEBUG_WM(F(""));
	_configPortalStart = millis();

	DEBUG_WM(F("Configuring access point... "));
	//DEBUG_WM(_apName);
	if (_apPassword != NULL) {
		if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
			// fail passphrase to short or long!
			DEBUG_WM(F("Invalid AccessPoint password. Ignoring"));
			_apPassword = NULL;
		}
		DEBUG_WM(_apPassword);
	}

	// optional soft ip config
	if (_ap_static_ip) {
		DEBUG_WM(F("Custom AP IP/GW/Subnet"));
		WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
	}

	// ssid with id
	// Serial.print("Check customID ");
	// Serial.println(_customIdElement);
	if (_customIdElement != "") {
		// Serial.println("not empty ");
		char temp[50];
		sprintf(temp, "%s_%s", _apName, _customIdElement);
		// Serial.println(temp);
		if (_apPassword != NULL) {
			WiFi.softAP(temp, _apPassword);   // password option
		} else {
			WiFi.softAP(temp);
		}
    DEBUG_WM(temp);
	} else {
		// Serial.println("empty ");
		if (_apPassword != NULL) {
			WiFi.softAP(_apName, _apPassword);   // password option
		} else {
			WiFi.softAP(_apName);
		}
    DEBUG_WM(_apName);
	}


	delay(500); // Without delay I've seen the IP address blank
	DEBUG_WM(F("AP IP address: "));
	DEBUG_WM(WiFi.softAPIP());

	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());

	/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
	server->on("/", std::bind(&WiFiManager::handleRoot, this));
	server->on("/wifi", std::bind(&WiFiManager::handleWifi, this, true));
	server->on("/0wifi", std::bind(&WiFiManager::handleWifi, this, false));
	server->on("/wifisave", std::bind(&WiFiManager::handleWifiSave, this));
	server->on("/i", std::bind(&WiFiManager::handleInfo, this));
	server->on("/r", std::bind(&WiFiManager::handleReset, this));
	server->on("/t", std::bind(&WiFiManager::handleToggle, this));
	server->on("/fwlink", std::bind(&WiFiManager::handleRoot, this)); // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
	//
	server->on("/u", std::bind(&WiFiManager::handleUpdate, this)); // handler for the /update form page
	server->on("/update", HTTP_POST,
	  std::bind(&WiFiManager::handleUpdateDone, this), std::bind(&WiFiManager::handleUpdating, this));
	server->onNotFound(std::bind(&WiFiManager::handleNotFound, this));
	server->begin(); // Web server start
	DEBUG_WM(F("HTTP server started"));
} // setupConfigPortal

void WiFiManager::handleToggle(){
	if (_lightToggleCallback != NULL) {
		_lightToggleCallback();
	}
	handleRoot();
}

boolean WiFiManager::autoConnect(){
	String ssid = "ESP" + String(ESP.getChipId());

	return autoConnect(ssid.c_str(), NULL);
}

boolean WiFiManager::autoConnect(char const * apName, char const * apPassword){
	// read eeprom for ssid and pass
	// String ssid = getSSID();
	// String pass = getPassword();

	// attempt to connect; should it fail, fall back to AP
	WiFi.mode(WIFI_STA);

	if (connectWifi("", "") == WL_CONNECTED) {
		DEBUG_WM(F("IP Address:"));
		DEBUG_WM(WiFi.localIP());
		// connected
		return true;
	}

	return startConfigPortal(apName, apPassword);
}

boolean WiFiManager::startConfigPortal(){
	String ssid = "ESP" + String(ESP.getChipId());

	return startConfigPortal(ssid.c_str(), NULL);
}

boolean WiFiManager::startConfigPortal(char const * apName, char const * apPassword){
	// setup AP
	WiFi.mode(WIFI_AP);
	DEBUG_WM("SET AP");

	_apName     = apName;
	_apPassword = apPassword;

	// notify we entered AP mode
	if (_apcallback != NULL) {
		_apcallback(this);
	}

	// ist das ne schlaue idee die hier on the fly zu laden?
	DEBUG_WM(F("Done"));

	connect = false;
	setupConfigPortal();

	uint8_t char_buffer;
	uint8_t * p;
	uint8_t f_p;
	uint8_t f_start;
	// uint32_t      t=millis();

	m_mqtt_sizes[0] = sizeof(m_mqtt->login);
	m_mqtt_sizes[1] = sizeof(m_mqtt->pw);
	m_mqtt_sizes[2] = sizeof(m_mqtt->dev_short);
	m_mqtt_sizes[3] = sizeof(m_mqtt->cap);
	m_mqtt_sizes[4] = sizeof(m_mqtt->server_ip);
	m_mqtt_sizes[5] = sizeof(m_mqtt->server_port);
	m_mqtt_sizes[6] = sizeof(m_mqtt->nw_ssid);
	m_mqtt_sizes[7] = sizeof(m_mqtt->nw_pw);

	f_p = 0;                  // pointer in dem struct
	p   = (uint8_t *) m_mqtt; // copy location

	while (_configPortalTimeout == 0 || millis() < _configPortalStart + _configPortalTimeout) {
		// ////////////// kolja serial config code ////////////
		// if(millis()-t>1000){
		//  t=millis();
		//  Serial.print("+");
		// }

		if (Serial.available()) {
			char_buffer = Serial.read();
			// Serial.print(char_buffer);
			// start char for config
			if (char_buffer == '?') {
				// Serial.print("@");
				p = (uint8_t *) m_mqtt;
				f_p         = 0;
				char_buffer = 13; // enter the "if" below
				f_start     = 0;
			}

			// jump to next field
			if (char_buffer == 13) {
				// Serial.print("#");
				f_start = 0;
				for (int i = 0; i <= sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]); i++) { // 1.2.3.4.5.6.7
					if (i > 0) {
						f_start += m_mqtt_sizes[i - 1];
					}
					// Serial.printf("+%i %i\r\n",f_p,f_start);
					// seach for the field that starts closes to our current pos
					if (f_p <= f_start) {
						for (int ii = 0; ii < f_start - f_p; ii++) { // add as many 0x00 to the config as required
							*p = 0x00;
							p++;
						}
						f_p = f_start; // set our new pos to the start of that field
						// print some shiny output
						if (i == 0) {
							Serial.print(F("\r\n\r\nStart readig config"));
						}
						;
						if (i >= 0 && i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0])) {
							explainMqttStruct(i, true);
						} else if (i == sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0])) { // last segement .. save and reboot
							// fill the buffer
							Serial.print(F("\r\n==========\r\nConfig stored\r\n"));
							explainFullMqttStruct(m_mqtt);
							Serial.print(F("==========\r\n"));
							// write to address 0 ++
							f_start = 0;
							for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]); i++) { // 1.2.3.4.5.6.7
								f_start += m_mqtt_sizes[i];
							}
							storeMqttStruct((char *) m_mqtt, f_start);
							delay(1000);
							// what about the wifi?
							Serial.print(F("Disconnect.\r\n"));
							WiFi.disconnect(false);
							Serial.print(F("Connect.\r\n"));
							WiFi.begin(m_mqtt->nw_ssid, m_mqtt->nw_pw);
							Serial.print(F("checking.\r\n"));
							if (WiFi.waitForConnectResult() != WL_CONNECTED) {
								Serial.print(F("Failed to connect."));
							} else {
								Serial.print(F("Connect ok. Restart now"));
								delay(500);
								ESP.restart();
							}
						}
						break; // leave loop
					}     // if(fp<f_start)
				} // loop over struct size
				  // set pointer to start of the field
				p  = (uint8_t *) m_mqtt;
				p += f_p;
			} else if (char_buffer == 127) { // backspace
				// search lowerlimit of this field
				f_start = 0;
				for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]); i++) { // 0.1.2.3.4.5.6.7
					// Serial.print("+");
					if (f_start + m_mqtt_sizes[i] > f_p) { // seach for the field that starts closes to our current pos
						break;
					}
					f_start += m_mqtt_sizes[i];
				}
				// Serial.printf("%i--%i\r\n",f_p,f_start);
				if (f_p > f_start) {
					p--; // limits?
					f_p--;
					Serial.print((char) 0x08); // geht das? ulkig aber ja
				}
			} else if (char_buffer != 10) { // plain char storing "\r"
				// Serial.print("&");
				// calc next segment
				f_start = 0;
				for (int i = 0; i < sizeof(m_mqtt_sizes) / sizeof(m_mqtt_sizes[0]); i++) { // 0.1.2.3.4.5.6.7
					// Serial.print("+");
					f_start += m_mqtt_sizes[i];
					if (f_p < f_start) {  // seach for the field that starts closes to our current pos
						break;
					}
				}
				// if(f_p<sizeof(*m_mqtt)-1){ // go on as long as we're in the structure
				if (f_p < f_start - 1) { // go on as long as we're in the structure
					// e.g.: first field is 16 byte long (f_start=16), we can use [0]..[14], [15] must be 0x00, so 13<16-1, 14<16-1; !! 15<16-1
					if (char_buffer != '\r' && char_buffer != '\n') {
						Serial.print((char) char_buffer);
					}
					*p = char_buffer; // store incoming char in struct
					p++;
					f_p++;
					// } else {
					//  Serial.println("L");
				}
			}
			connect = false;
		}
		// ////////////// kolja serial config code ////////////

		// DNS
		dnsServer->processNextRequest();

		// kolja notes: im wesentlichen warten wir auf den aufruf von /wifisave
		// das ruf die fkt handleWifiSave auf die dann die argumente
		// einsammelt, "s" und "p" sind ssid und passwort die dann in _ssid und _pass
		// landen, ACHTUNG nicht "ip" als custom parameter nutzen, das ist die feste IP
		// wenn die parameter gespeichert sind wird connect auf true gesetzt und die
		// if da unten geht los und verbindet uns. das SDK speichert die zuletzt erstellte
		// verbindung und stellt die sogar von alleien beim naechste reboot her, daher
		// speichern wir die ssid nicht explizit selbst.
		server->handleClient();


		if (connect) {
			connect = false;
			delay(2000);
			DEBUG_WM(F("*WM: Connecting to new AP"));

			// using user-provided  _ssid, _pass in place of system-stored ssid and pass
			if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
				DEBUG_WM(F("Failed to connect."));
			} else {
				// connected
				WiFi.mode(WIFI_STA);
				// notify that configuration has changed and any optional parameters should be saved
				if (_savecallback != NULL) {
					// todo: check if any custom parameters actually exist, and check if they really changed maybe
					_savecallback();
				}
				break;
			}

			if (_shouldBreakAfterConfig) {
				// flag set to exit after config after trying to connect
				// notify that configuration has changed and any optional parameters should be saved
				if (_savecallback != NULL) {
					// todo: check if any custom parameters actually exist, and check if they really changed maybe
					_savecallback();
				}
				break;
			}
		}
		// yield();
	}
	Serial.println(F("*WM: end startConfigPortal"));
	server.reset();
	dnsServer.reset();
	WiFi.mode(WIFI_STA);

	return WiFi.status() == WL_CONNECTED;
} // startConfigPortal

boolean WiFiManager::storeMqttStruct(char * temp, uint8_t size){
	uint8_t checksum = CHK_FORMAT_V2;

	for (int i = 0; i < size; i++) {
		EEPROM.write(i, *temp);
		// Serial.print(int(*temp));
		checksum ^= *temp;
		temp++;
	}
	EEPROM.write(size, checksum);
	EEPROM.commit();
	return true;
}

boolean WiFiManager::loadMqttStruct(char * temp, uint8_t size){
	char* temp2=temp;
	EEPROM.begin(512); // can be up to 4096
	uint8_t checksum = CHK_FORMAT_V2;
	for (int i = 0; i < size; i++) {
		*temp = EEPROM.read(i);
		//Serial.printf(" (%i)",i);
		//Serial.print(char(*temp));
		checksum ^= *temp;
		temp++;
	}
	checksum ^= EEPROM.read(size);
	if (checksum == 0x00) {
		// Serial.println("EEok");
		return true;
	} else {
		Serial.println("EEfail");

		// try legacy V1 to v2 conversion
		temp = temp2;        // rewind
		for(int i=0;i<(16+16+6+20+16+6);i++){
			//Serial.printf(" (%i)",i);
			//Serial.print(char(*temp));
			if(i<16+16+6){
				*temp = EEPROM.read(i);
				temp++;
			} else if(i == 16+16+6){
				*temp = '0';         // write cap
				temp++;
				*temp= 0;
				temp++;
			} else if(i >= 16+16+6+20){
				*temp = EEPROM.read(i);
				temp++;
			}
		}
		temp = temp2;        // rewind
		Serial.println(F("///////////////////////"));
		Serial.println(F("EEPROM read failed, try legacy"));
		explainFullMqttStruct((mqtt_data*)temp);
		Serial.println(F("saving to EEPROM with new struct"));
		Serial.println(F("///////////////////////"));

		storeMqttStruct(temp, size);
		// try legacy V1 to v2 conversion
	}
	return false;
}

void WiFiManager::explainFullMqttStruct(mqtt_data * mqtt){
	explainMqttStruct(0, false);
	Serial.println(mqtt->login);
	explainMqttStruct(1, false);
	Serial.println(mqtt->pw);
	explainMqttStruct(2, false);
	Serial.println(mqtt->dev_short);
	explainMqttStruct(3, false);
	Serial.println(mqtt->cap);
	explainMqttStruct(4, false);
	Serial.println(mqtt->server_ip);
	explainMqttStruct(5, false);
	Serial.println(mqtt->server_port);
}

void WiFiManager::explainMqttStruct(uint8_t i, boolean rn){
	if (rn) {
		Serial.print(F("\r\n"));
	}
	if (i == 0) {
		Serial.print(F("MQTT login: "));
	} else if (i == 1) {
		Serial.print(F("MQTT pw: "));
	} else if (i == 2) {
		Serial.print(F("MQTT dev_short: "));
	} else if (i == 3) {
		Serial.print(F("MQTT Capability: "));
	} else if (i == 4) {
		Serial.print(F("MQTT server IP: "));
	} else if (i == 5) {
		Serial.print(F("MQTT server port: "));
	} else if (i == 6) {
		Serial.print(F("Network SSID: "));
	} else if (i == 7) {
		Serial.print(F("Network PW: "));
	} else {
		Serial.print(F("ERROR 404 "));
	}
	if (rn) {
		Serial.print(F("\r\n"));
	}
};

int WiFiManager::connectWifi(String ssid, String pass){
	//DEBUG_WM(F("Connecting as wifi client..."));
	Serial.print(F("*WM: Connecting to "));
	Serial.println(WiFi.SSID());

	// check if we've got static_ip settings, if we do, use those.
	if (_sta_static_ip) {
		DEBUG_WM(F("Custom STA IP/GW/Subnet"));
		WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
		DEBUG_WM(WiFi.localIP());
	}
	// fix for auto connect racing issue
	if (WiFi.status() == WL_CONNECTED) {
		DEBUG_WM("Already connected. Bailing out.");
		return WL_CONNECTED;
	}
	// check if we have ssid and pass and force those, if not, try with last saved values
	if (ssid != "") {
		WiFi.begin(ssid.c_str(), pass.c_str());
	} else {
		if (WiFi.SSID()) {

			// trying to fix connection in progress hanging
			ETS_UART_INTR_DISABLE();
			wifi_station_disconnect();
			ETS_UART_INTR_ENABLE();

			WiFi.begin();
		} else {
			DEBUG_WM("No saved credentials");
		}
	}

	int connRes = waitForConnectResult();
	DEBUG_WM("Connection result: ");
	DEBUG_WM(connRes);
	// not connected, WPS enabled, no pass - first attempt
	if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
		startWPS();
		// should be connected at the end of WPS
		connRes = waitForConnectResult();
	}
	return connRes;
} // connectWifi

uint8_t WiFiManager::waitForConnectResult(){
	if (_connectTimeout == 0) {
		return WiFi.waitForConnectResult();
	} else {
		Serial.print(F("Waiting for connection result with time out "));
		Serial.println((uint16_t)(_connectTimeout/1000));
		unsigned long start    = millis();
		boolean keepConnecting = true;
		uint8_t status;
		while (keepConnecting) {
			status = WiFi.status();
			if (millis() > start + _connectTimeout) {
				keepConnecting = false;
				DEBUG_WM(F("Connection timed out"));
			}
			if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
				keepConnecting = false;
			}
			delay(100);
		}
		return status;
	}
}

void WiFiManager::setMqtt(mqtt_data * mqtt){
	EEPROM.begin(512); // can be up to 4096
	m_mqtt = mqtt;
}

void WiFiManager::startWPS(){
	DEBUG_WM("START WPS");
	WiFi.beginWPSConfig();
	DEBUG_WM("END WPS");
}

/*
 * String WiFiManager::getSSID() {
 * if (_ssid == "") {
 *  DEBUG_WM(F("Reading SSID"));
 *  _ssid = WiFi.SSID();
 *  DEBUG_WM(F("SSID: "));
 *  DEBUG_WM(_ssid);
 * }
 * return _ssid;
 * }
 *
 * String WiFiManager::getPassword() {
 * if (_pass == "") {
 *  DEBUG_WM(F("Reading Password"));
 *  _pass = WiFi.psk();
 *  DEBUG_WM("Password: " + _pass);
 *  //DEBUG_WM(_pass);
 * }
 * return _pass;
 * }
 */
String WiFiManager::getConfigPortalSSID(){
	return _apName;
}

void WiFiManager::resetSettings(){
	DEBUG_WM(F("settings invalidated"));
	DEBUG_WM(F("THIS MAY CAUSE AP NOT TO START UP PROPERLY. YOU NEED TO COMMENT IT OUT AFTER ERASING THE DATA."));
	WiFi.disconnect(true);
	// delay(200);
}

void WiFiManager::setTimeout(unsigned long seconds){
	setConfigPortalTimeout(seconds);
}

void WiFiManager::setConfigPortalTimeout(unsigned long seconds){
	_configPortalTimeout = seconds * 1000;
}

void WiFiManager::setConnectTimeout(unsigned long seconds){
	_connectTimeout = seconds * 1000;
}

void WiFiManager::setDebugOutput(boolean debug){
	_debug = debug;
}

void WiFiManager::setAPStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn){
	_ap_static_ip = ip;
	_ap_static_gw = gw;
	_ap_static_sn = sn;
}

void WiFiManager::setSTAStaticIPConfig(IPAddress ip, IPAddress gw, IPAddress sn){
	_sta_static_ip = ip;
	_sta_static_gw = gw;
	_sta_static_sn = sn;
}

void WiFiManager::setMinimumSignalQuality(int quality){
	_minimumQuality = quality;
}

void WiFiManager::setBreakAfterConfig(boolean shouldBreak){
	_shouldBreakAfterConfig = shouldBreak;
}

/** Handle  update doc */
void WiFiManager::handleUpdate(){
	DEBUG_WM(F("Handle update"));
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Options");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += "<h1>";
	page += _apName;
	page += "</h1>";
	page += F("<h3>WiFiManager</h3>");
	page += FPSTR(HTTP_UPDATE);
	page += FPSTR(HTTP_END);
	server->send(200, "text/html", page);
}

void WiFiManager::handleUpdating(){
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}
	// handler for the file upload, get's the sketch bytes, and writes
	// them through the Update object
	HTTPUpload& upload = server->upload();
	if (upload.status == UPLOAD_FILE_START) {
		Serial.setDebugOutput(true);

		WiFiUDP::stopAll();
		Serial.printf("Update: %s\r\n", upload.filename.c_str());
		uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
		if (!Update.begin(maxSketchSpace)) { // start with max available size
			Update.printError(Serial);
		}
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		Serial.printf(".");
		if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
			Update.printError(Serial);
		}
	} else if (upload.status == UPLOAD_FILE_END) {
		if (Update.end(true)) { // true to set the size to the current progress
			Serial.printf("Update Success: %u\r\nRebooting...\r\n", upload.totalSize);
		} else {
			Update.printError(Serial);
		}
		Serial.setDebugOutput(false);
	} else if (upload.status == UPLOAD_FILE_ABORTED) {
		Update.end();
		Serial.println("Update was aborted");
	}
	delay(0);
} // handleUpdating

void WiFiManager::handleUpdateDone(){
	DEBUG_WM(F("Handle update done"));
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}
	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Options");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += "<h1>";
	page += _apName;
	page += "</h1>";
	page += F("<h3>WiFiManager</h3>");
	if (Update.hasError()) {
		page += FPSTR(HTTP_UPDATE_FAI);
		Serial.println("update failed");
	} else {
		page += FPSTR(HTTP_UPDATE_SUC);
		Serial.println("update done");
	}
	page += FPSTR(HTTP_END);
	server->send(200, "text/html", page);
	delay(1000); // send page
	ESP.restart();
}

/** Handle root or redirect to captive portal */
void WiFiManager::handleRoot(){
	DEBUG_WM(F("Handle root"));
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Options");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += "<h1>";
	page += _apName;
	page += "</h1>";
	page += F("<h3>WiFiManager</h3>");
	page += FPSTR(HTTP_PORTAL_OPTIONS);
	page += FPSTR(HTTP_END);

	server->send(200, "text/html", page);
}

/** Wifi config page handler */
void WiFiManager::handleWifi(boolean scan){
	String page = FPSTR(HTTP_HEAD);

	page.replace("{v}", "Config ESP");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");

	if (scan) {
		int n = WiFi.scanNetworks();
		DEBUG_WM(F("Scan done"));
		if (n == 0) {
			DEBUG_WM(F("No networks found"));
			page += F("No networks found. Refresh to scan again.");
		} else {
			// sort networks
			int indices[n];
			for (int i = 0; i < n; i++) {
				indices[i] = i;
			}

			// RSSI SORT

			// old sort
			for (int i = 0; i < n; i++) {
				for (int j = i + 1; j < n; j++) {
					if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
						std::swap(indices[i], indices[j]);
					}
				}
			}

			/*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
			 * {
			 * return WiFi.RSSI(a) > WiFi.RSSI(b);
			 * });*/

			// remove duplicates ( must be RSSI sorted )
			if (_removeDuplicateAPs) {
				String cssid;
				for (int i = 0; i < n; i++) {
					if (indices[i] == -1) continue;
					cssid = WiFi.SSID(indices[i]);
					for (int j = i + 1; j < n; j++) {
						if (cssid == WiFi.SSID(indices[j])) {
							DEBUG_WM("DUP AP: " + WiFi.SSID(indices[j]));
							indices[j] = -1; // set dup aps to index -1
						}
					}
				}
			}

			// display networks in page
			for (int i = 0; i < n; i++) {
				if (indices[i] == -1) continue;  // skip dups
				DEBUG_WM(WiFi.SSID(indices[i]));
				DEBUG_WM(WiFi.RSSI(indices[i]));
				int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

				if (_minimumQuality == -1 || _minimumQuality < quality) {
					String item = FPSTR(HTTP_ITEM);
					String rssiQ;
					rssiQ += quality;
					item.replace("{v}", WiFi.SSID(indices[i]));
					item.replace("{r}", rssiQ);
					if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
						item.replace("{i}", "l");
					} else {
						item.replace("{i}", "");
					}
					// DEBUG_WM(item);
					page += item;
					delay(0);
				} else {
					DEBUG_WM(F("Skipping due to quality"));
				}
			}
			page += "<br/>";
		}
	}

	page += FPSTR(HTTP_FORM_START);
	char parLength[2];
	// add the extra parameters to the form
	for (int i = 0; i < _paramsCount; i++) {
		if (_params[i] == NULL) {
			break;
		}

		String pitem;
		if (_params[i]->getID() != NULL) {
			if (!_params[i]->getType()) {   // regular parameter
				pitem = FPSTR(HTTP_FORM_PARAM);
				pitem.replace("{c}", _params[i]->getCustomHTML());
			} else { // boolean parameter
				pitem = FPSTR(HTTP_FORM_BOOL_PARAM);
				if (*(_params[i]->getValue()) != 0 && *(_params[i]->getValue()) != '0') { // not 0 and not '0'
					pitem.replace("{c}", "checked");
				} else {
					pitem.replace("{c}", "");
				}
			}
			pitem.replace("{i}", _params[i]->getID());
			pitem.replace("{n}", _params[i]->getID());
			pitem.replace("{p}", _params[i]->getPlaceholder());
			snprintf(parLength, 2, "%d", _params[i]->getValueLength());
			pitem.replace("{l}", parLength);
			pitem.replace("{v}", _params[i]->getValue());
		} else {
			pitem = _params[i]->getCustomHTML();
		}
		page += pitem;
	}
	if (_params[0] != NULL) {
		page += "<br/>";
	}

	if (_sta_static_ip) {
		String item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "ip");
		item.replace("{n}", "ip");
		item.replace("{p}", "Static IP");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_ip.toString());

		page += item;

		item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "gw");
		item.replace("{n}", "gw");
		item.replace("{p}", "Static Gateway");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_gw.toString());

		page += item;

		item = FPSTR(HTTP_FORM_PARAM);
		item.replace("{i}", "sn");
		item.replace("{n}", "sn");
		item.replace("{p}", "Subnet");
		item.replace("{l}", "15");
		item.replace("{v}", _sta_static_sn.toString());

		page += item;

		page += "<br/>";
	}

	page += FPSTR(HTTP_FORM_END);
	page += FPSTR(HTTP_SCAN_LINK);

	page += FPSTR(HTTP_END);

	server->send(200, "text/html", page);


	DEBUG_WM(F("Sent config page"));
} // handleWifi

/** Handle the WLAN save form and redirect to WLAN config page again */
void WiFiManager::handleWifiSave(){
	DEBUG_WM(F("WiFi save"));

	// SAVE/connect here
	_ssid = server->arg("s").c_str();
	_pass = server->arg("p").c_str();

	// parameters
	for (int i = 0; i < _paramsCount; i++) {
		if (_params[i] == NULL) {
			break;
		}
		// read parameter
		String value = server->arg(_params[i]->getID()).c_str();
		// store it in array
		value.toCharArray(_params[i]->_value, _params[i]->_length);
		DEBUG_WM(F("Parameter"));
		DEBUG_WM(_params[i]->getID());
		DEBUG_WM(value);
	}

	if (server->arg("ip") != "") {
		DEBUG_WM(F("static ip"));
		DEBUG_WM(server->arg("ip"));
		// _sta_static_ip.fromString(server->arg("ip"));
		String ip = server->arg("ip");
		optionalIPFromString(&_sta_static_ip, ip.c_str());
	}
	if (server->arg("gw") != "") {
		DEBUG_WM(F("static gateway"));
		DEBUG_WM(server->arg("gw"));
		String gw = server->arg("gw");
		optionalIPFromString(&_sta_static_gw, gw.c_str());
	}
	if (server->arg("sn") != "") {
		DEBUG_WM(F("static netmask"));
		DEBUG_WM(server->arg("sn"));
		String sn = server->arg("sn");
		optionalIPFromString(&_sta_static_sn, sn.c_str());
	}

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Credentials Saved");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += FPSTR(HTTP_SAVED);
	page += FPSTR(HTTP_END);

	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent wifi save page"));

	connect = true; // signal ready to connect/reset
} // handleWifiSave

/** Handle the info page */
void WiFiManager::handleInfo(){
	DEBUG_WM(F("Info"));

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += F("<dl>");
	page += F("<dt>Chip ID</dt><dd>");
	page += ESP.getChipId();
	page += F("</dd>");
	page += F("<dt>Flash Chip ID</dt><dd>");
	page += ESP.getFlashChipId();
	page += F("</dd>");
	page += F("<dt>IDE Flash Size</dt><dd>");
	page += ESP.getFlashChipSize();
	page += F(" bytes</dd>");
	page += F("<dt>Real Flash Size</dt><dd>");
	page += ESP.getFlashChipRealSize();
	page += F(" bytes</dd>");
	page += F("<dt>Soft AP IP</dt><dd>");
	page += WiFi.softAPIP().toString();
	page += F("</dd>");
	page += F("<dt>Soft AP MAC</dt><dd>");
	page += WiFi.softAPmacAddress();
	page += F("</dd>");
	page += F("<dt>Station MAC</dt><dd>");
	page += WiFi.macAddress();
	page += F("</dd>");
	page += F("</dl>");
	page += FPSTR(HTTP_END);

	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent info page"));
} // handleInfo

/** Handle the reset page */
void WiFiManager::handleReset(){
	DEBUG_WM(F("Reset"));

	String page = FPSTR(HTTP_HEAD);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEAD_END);
	page += FPSTR("<h1>CLI ");
	page += _customIdElement;
	page += FPSTR("</h1><br>");
	page += F("Module will reset in a few seconds.");
	page += FPSTR(HTTP_END);
	server->send(200, "text/html", page);

	DEBUG_WM(F("Sent reset page"));
	delay(5000);
	ESP.reset();
	delay(2000);
}

// removed as mentioned here https://github.com/tzapu/WiFiManager/issues/114

/*void WiFiManager::handle204() {
 * DEBUG_WM(F("204 No Response"));
 * server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
 * server->sendHeader("Pragma", "no-cache");
 * server->sendHeader("Expires", "-1");
 * server->send ( 204, "text/plain", "");
 * }*/

void WiFiManager::handleNotFound(){
	if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
		return;
	}
	String message = "File Not Found\r\n\r\n";
	message += "URI: ";
	message += server->uri();
	message += "\r\nMethod: ";
	message += ( server->method() == HTTP_GET ) ? "GET" : "POST";
	message += "\r\nArguments: ";
	message += server->args();
	message += "\r\n";

	for (uint8_t i = 0; i < server->args(); i++) {
		message += " " + server->argName(i) + ": " + server->arg(i) + "\r\n";
	}
	server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	server->sendHeader("Pragma", "no-cache");
	server->sendHeader("Expires", "-1");
	server->send(404, "text/plain", message);
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean WiFiManager::captivePortal(){
	if (!isIp(server->hostHeader()) ) {
		DEBUG_WM(F("Request redirected to captive portal"));
		server->sendHeader("Location", String("http://") + toStringIp(server->client().localIP()), true);
		server->send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
		server->client().stop();             // Stop is needed because we sent no content length
		return true;
	}
	return false;
}

// start up config portal callback
void WiFiManager::setAPCallback(void (* func)(WiFiManager * myWiFiManager) ){
	_apcallback = func;
}

// start up save config callback
void WiFiManager::setSaveConfigCallback(void (* func)(void) ){
	_savecallback = func;
}

void WiFiManager::setLightToggleCallback( void (*func)(void) ){
	_lightToggleCallback = func;
}

// sets a custom element to add to head, like a new style tag
void WiFiManager::setCustomHeadElement(const char * element){
	_customHeadElement = element;
}

// sets a custom element to add to Body, like a new style tag
void WiFiManager::setCustomIdElement(const char * element){
	_customIdElement = element;
}

// if this is true, remove duplicated Access Points - defaut true
void WiFiManager::setRemoveDuplicateAPs(boolean removeDuplicates){
	_removeDuplicateAPs = removeDuplicates;
}

template <typename Generic>
void WiFiManager::DEBUG_WM(Generic text){
	if (_debug) {
		Serial.print("[WM] ");
		Serial.println(text);
	}
}

int WiFiManager::getRSSIasQuality(int RSSI){
	int quality = 0;

	if (RSSI <= -100) {
		quality = 0;
	} else if (RSSI >= -50) {
		quality = 100;
	} else {
		quality = 2 * (RSSI + 100);
	}
	return quality;
}

/** Is this an IP? */
boolean WiFiManager::isIp(String str){
	for (int i = 0; i < str.length(); i++) {
		int c = str.charAt(i);
		if (c != '.' && (c < '0' || c > '9')) {
			return false;
		}
	}
	return true;
}

/** IP to String? */
String WiFiManager::toStringIp(IPAddress ip){
	String res = "";

	for (int i = 0; i < 3; i++) {
		res += String((ip >> (8 * i)) & 0xFF) + ".";
	}
	res += String(((ip >> 8 * 3)) & 0xFF);
	return res;
}
