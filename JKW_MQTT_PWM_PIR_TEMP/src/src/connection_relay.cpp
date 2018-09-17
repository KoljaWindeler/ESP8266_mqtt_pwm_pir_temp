#include "connection_relay.h"

connection_relay::connection_relay(){
	// reset clients
	for (int i = 0; i < ESP8266_NUM_CLIENTS; i++) {
		espClients[i]         = NULL;
		espClientsLastComm[i] = 0;
	}
	espLastcomm       = 0;
	m_AP_running      = false;
	m_connection_type = CONNECTION_DIRECT_CONNECTED; // wifi STA connection can survive reboots
	m_ota_status      = OTA_STATUS_END_FAILED;       // avoid start in middle

	// root node with own MAC, kind of pointless but we have to start somewhere
	uint8_t mac[6];
	WiFi.macAddress(mac);
	bl = new blacklist_entry(mac);

	// mesh is a bit tricky. first test failed misably, due to super long chaining
	// 8 devices lined up in a row and the connection was bad.
	network.setMeshMode(MESH_MODE_CLIENT_ONLY); // clever?
};

connection_relay::~connection_relay(){ };

// scan can return [blocking] the nr of APs
int8_t connection_relay::scan(bool blocking){
	int8_t status;

	if (blocking) {
		WiFi.scanDelete();
		status = WiFi.scanNetworks();
		return status;
	} else {
		status = WiFi.scanComplete();
		if (status == -2) { // scan not triggered
			WiFi.scanNetworks(true);
			return 0;                 // not complete
		} else if (status == -1) { // scan still ongoing
			return 0;                 // not complete
		}
		return status; // complete
	}
	return 0;
}

// we are the last node and should directly connect to the WIFI
bool connection_relay::DirectConnect(){
	logger.println(TOPIC_WIFI, F("Currently not connected, initiate direct connection ..."), COLOR_RED);
	logger.println(TOPIC_WIFI, F("Removing  all old credentials ..."), COLOR_YELLOW);
	// mqtt.nw_ssid, mqtt.nw_pw or autoconnect?
	WiFi.disconnect();
	logger.print(TOPIC_WIFI, F("Connecting to: "), COLOR_YELLOW);
	logger.pln(mqtt.nw_ssid);

	if (wifiManager.connectWifi(mqtt.nw_ssid, mqtt.nw_pw) == WL_CONNECTED) {
		sprintf_P(m_msg_buffer,PSTR("Direct connection with %s estabilshed, IP: %i.%i.%i.%i"),
			(char*)WiFi.SSID().c_str(),WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
		logger.println(TOPIC_WIFI, m_msg_buffer, COLOR_GREEN);
		m_connection_type = CONNECTION_DIRECT_CONNECTED;
		return true;
	}
	return false;
}

// enable or disable mesh function
void connection_relay::setMeshMode(uint8_t in){
	m_mesh_mode = in;
}

// get status, is mesh ok or shall we work standalone?
uint8_t connection_relay::getMeshMode(){
	return m_mesh_mode;
}

// direct wifi didn't work, connect via mesh
bool connection_relay::MeshConnect(){
	int8_t scan_count = scan(true); // will start with blocking scan
	int8_t best_AP  = -1;
	uint8_t best_sig = 0;

	if (scan_count > 0) {
		// erial.printf("%d network(s) found\r\n", scan_count);
		for (int i = 0; i < scan_count; i++) {
			// erial.printf("%d: %s, Ch:%d (%ddBm) %s %i\n\r", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "", WiFi.isHidden(i));
			// if(WiFi.isHidden(i) && strncmp(WiFi.SSID(i).c_str(),AP_SSID,strlen(AP_SSID))==0){
			if (strncmp(WiFi.SSID(i).c_str(), AP_SSID, strlen(AP_SSID)) == 0) {
				// erial.println("interesting config");
				// schema: AP_SSID-dev1-2 = AP is dev1, level = 2
				uint8_t step = 0;
				uint8_t this_mesh_level = 1;
				uint8_t this_sig = wifiManager.getRSSIasQuality((int)WiFi.RSSI(i));

				if(WiFi.SSID(i).charAt(strlen(WiFi.SSID(i).c_str())-2)=='L'){
					this_mesh_level = WiFi.SSID(i).charAt(strlen(WiFi.SSID(i).c_str())-1);
					if(this_mesh_level>'0' && this_mesh_level<='9'){
						this_mesh_level-='0';
						this_sig = this_sig/2 + this_sig/(2<<(this_mesh_level-1)); // 1. level 100%, 2. 75%, 3. 62,5%
					} else {
						this_mesh_level = 0; // can't decode
					}
				}
			 	Serial.printf("%d: %s, Ch:%d (%ddBm -> %d) %s %i\n\r", i+1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), this_sig, WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "", WiFi.isHidden(i));

				if (best_AP == -1 || this_sig>best_sig) {
					if (bl->get_fails(WiFi.BSSID(i)) < 2) { // only connect to this network if there are less then 2 failed tries
						// erial.println("actually the best");
						best_sig = this_sig;
						m_mesh_level = this_mesh_level;
						best_AP = i;
					}
				}
			}
		}

		if (best_AP > -1) {
			sprintf_P(m_msg_buffer, PSTR("%i Networks found, incl mesh '%s' [%idbm]. Connecting"), scan_count, WiFi.SSID(best_AP).c_str(),
			  WiFi.RSSI(best_AP));
			logger.println(TOPIC_WIFI, m_msg_buffer, COLOR_GREEN);

			// connect to AP
			WiFi.persistent(false); // avoids updating the same flash cell every reboot
			WiFi.begin(WiFi.SSID(best_AP).c_str(), AP_PW, WiFi.channel(best_AP), WiFi.BSSID(best_AP), true);
			WiFi.waitForConnectResult();
			if (WiFi.status() == WL_CONNECTED) {
				sprintf_P(m_msg_buffer, PSTR("Mesh connection established %i.%i.%i.%i"), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP(
				  )[2], WiFi.localIP()[3]);
				logger.println(TOPIC_WIFI, m_msg_buffer, COLOR_GREEN);
				m_connection_type = CONNECTION_MESH_CONNECTED;
				return true;
			} else {
				sprintf_P(m_msg_buffer, PSTR("Mesh connection failed"));
				logger.println(TOPIC_WIFI, m_msg_buffer, COLOR_RED);
			}
		} else {
			sprintf_P(m_msg_buffer, PSTR("No mesh node (%s) in range"), AP_SSID);
			logger.println(TOPIC_WIFI, m_msg_buffer, COLOR_RED);
		}
	}
	return false;
} // MeshConnect

// some sort of wifi connection is there, connect to mqtt.
// this can happen directly or via mesh proxy thing
// if mesh connected: Blocking call for server!
bool connection_relay::connectServer(char * dev_short, char * login, char * pw){
	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		logger.println(TOPIC_MQTT, F("Establishing direct MQTT link"), COLOR_YELLOW);

		client.setServer(mqtt.server_ip, atoi(mqtt.server_port));
		client.setCallback(callback); // in main.cpp
		return client.connect(dev_short, login, pw, build_topic("INFO", UNIT_TO_PC), 0, true, "lost signal");
	} else if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		logger.println(TOPIC_MQTT, F("Establishing indirect MESH-MQTT link"), COLOR_YELLOW);

		IPAddress apIP = WiFi.localIP();
		apIP[3] = 1;

		// erial.println("A..");
		// delay(100);

		if (espUplink.connect(apIP, MESH_PORT)) {
			// erial.println("B..");
			// delay(100);
			if (send_up(dev_short, 0)) { // only read
				// erial.println("C..");
				bl->set_fails(WiFi.BSSID(), 0);
				// delay(100);
				logger.println(TOPIC_MQTT, F("Received server welcome"), COLOR_GREEN);
			} else {
				logger.println(TOPIC_MQTT, F("Didn't receive server welcome"), COLOR_YELLOW);
			}
			return true; // return true, even when we didn't receive welcome. good?
		} else {
			logger.println(TOPIC_MQTT, F("Server didn't accept connection"), COLOR_RED);
			bl->set_fails(WiFi.BSSID(), 1); // increase blacklist counter by one
			disconnectServer();             // stop wifi to force reconnect
		}
	}
	return false;
} // connectServer

// suppose to send a raw message around in the network
// second purpose is to start the rounting message which will concat all station on the way
bool connection_relay::loopCheck(){
	if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		logger.println(TOPIC_CON_REL, F("sending NETLOOP check up the chain"), COLOR_PURPLE);

		char msg[50];
		uint8_t mac[6];
		WiFi.macAddress(mac);
		sprintf_P(msg, PSTR("%c%02x:%02x:%02x:%02x:%02x:%02x"), MSG_TYPE_NW_LOOP_CHECK, mac[5], mac[4], mac[3], mac[2], mac[1],
		  mac[0]);
		enqueue_up(msg, strlen(msg));
		// send_up(msg,strlen(msg));
	}
	;
};

// publish the routing
bool connection_relay::publishRouting(){
	char msg[50];
	char topic[50];

	sprintf_P(topic, PSTR("%s"), build_topic("routing", UNIT_TO_PC));
	sprintf_P(msg, PSTR("%c%c%c%s%c%s"), MSG_TYPE_ROUTING, strlen(topic) + 1, strlen(
	   mqtt.dev_short) + 1, topic, 0, mqtt.dev_short);

	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		return publish((char *) topic, mqtt.dev_short);
	} else {
		return enqueue_up(msg, 3 + strlen(topic) + 1 + strlen(mqtt.dev_short) + 1);
	}
};

// disconnect e.g. to apply new setting before reconnecting
void connection_relay::disconnectServer(){
	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		client.disconnect();
	} else if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		if (espUplink) {
			espUplink.stop();
		}
		WiFi.disconnect(false); // stop wifi to force reconnect
	}
}

// make "connected" more versatile
bool connection_relay::connected(){ return connected(true); }

bool connection_relay::connected(bool print){
	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		if (!client.connected()) {
			if (print) {
				logger.println(TOPIC_CON_REL, F("Not connected to MQTT"), COLOR_RED);
			}
			return false;
		}
		return true;
	} else if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		if (!espUplink.connected()) {
			if (print) {
				logger.println(TOPIC_CON_REL, F("Not connected to mesh"), COLOR_RED);
			}
			return false;
		}
		return true;
	}
	logger.println(TOPIC_CON_REL, F("unknown connection type"), COLOR_RED);
	return false;
}

// replacement for the subscribe routine.
// if direct connected simply a mqtt subscribe
// else it will preformat the message
bool connection_relay::subscribe(char * topic){ return subscribe(topic, false); }

bool connection_relay::subscribe(char * topic, bool enqueue){
	if (!connected()) {
		return false;
	}
	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		// erial.print("Direct subscibe: ");
		// erial.println(topic);
		client.subscribe(topic); // MQTT_TRACE_TOPIC topic
		for (uint8_t i; i < 10; i++) {
			client.loop();
		}
		return connected();
	} else if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		uint16_t len = strlen(topic) + 2;
		char msg[len];
		msg[0] = MSG_TYPE_SUBSCRPTION;
		memcpy(msg + 1, topic, strlen(topic));
		msg[strlen(topic) + 1] = '\0';
		// send or enqueue
		if (enqueue) {
			return enqueue_up(msg, len);
		} else {
			return send_up(msg, len);
		}
	}
	return false;
}

// replacement for the publish routine.
// if direct connected simply a mqtt publish
// else it will preformat the message
bool connection_relay::publish(char * topic, char * mqtt_msg){  return publish(topic, mqtt_msg, false); }

bool connection_relay::publish(char * topic, char * mqtt_msg, bool enqueue){
	if (!connected()) {
		return false;
	}
	if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		client.publish(topic, mqtt_msg, true);
		client.loop();
		client.loop();
		client.loop();
		return client.connected();
	} else if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		uint16_t len = strlen(topic) + strlen(mqtt_msg) + 5;
		char msg[len];
		// a bit strange to add 0x00 in the middle of a string, but help to display this later
		sprintf_P(msg, PSTR("%c%c%c%s%c%s"), MSG_TYPE_PUBLISH, strlen(topic) + 1, strlen(mqtt_msg) + 1, topic, 0, mqtt_msg); // last 0x00 will be added automatically

		// for(uint8_t i=0; i<strlen(topic)+strlen(mqtt_msg)+5; i++){
		//	.printf("[%i] %c %i\r\n",i,msg[i],msg[i]);
		// }
		// send or enqueue
		if (enqueue) {
			return enqueue_up(msg, len);
		} else {
			return send_up(msg, len);
		}
	}
	return false;
}

// used to send messages from the mqtt server downstream to all connected clients
bool connection_relay::broadcast_publish_down(char * topic, char * mqtt_msg, uint16_t payload_size){
	uint8_t client_count = 0;

	for (int i = 0; i < ESP8266_NUM_CLIENTS; i++) {
		if (espClients[i]) {
			client_count++;
		}
	}
	if (client_count == 0) {
		return true; // no clients to serve ..
	}
	// hmm no better way to combine the two buffer, but creating a new one?
	char msg[strlen(topic) + payload_size + 5];
	msg[0] = MSG_TYPE_PUBLISH;
	msg[1] = strlen(topic);
	msg[2] = payload_size / 256;
	msg[3] = payload_size % 256;
	memcpy(msg + 4, topic, strlen(topic)); // after this the topic is not longer \0 terminated
	memcpy(msg + 4 + strlen(topic), mqtt_msg, payload_size);
	uint16_t msg_size = strlen(topic) + payload_size + 5;
	msg[msg_size - 1] = '\0';

	if (msg_size < 100) {
		sprintf_P(m_msg_buffer, PSTR("(down) FWD '%s' -> '%s'"), topic, mqtt_msg);
	} else {
		sprintf_P(m_msg_buffer, PSTR("(down) FWD '%s' %i bytes"), topic, msg_size);
	}
	logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);

	for (int i = 0; i < ESP8266_NUM_CLIENTS; i++) {
		if (espClients[i]) {
			espClients[i]->write(msg, msg_size);
		}
	}
	return true;
} // broadcast_publish_down

// send pre-encoded messages (subscriptions/pubish's) UP to our mesh server
bool connection_relay::send_up(char * msg, uint16_t size){
	// erial.println("1..");
	// delay(100);
	if (!connected()) {
		return false;
	}
	uint32_t start = millis();
	uint8_t buf;
	// send up
	// erial.println("2..");
	// delay(100);

	espUplink.write(msg, size);

	// erial.println("3..");
	// delay(100);

	// waitup to 5 sec for ACK
	while ((millis() - start < 5000) || espUplink.available()) {
		// erial.println(millis()-start);
		delay(1); // resets wtd
		if (espUplink.available()) {
			// erial.println("4..");
			// delay(100);
			espUplink.read(&buf, 1);
			// erial.printf("recv %i %c\r\n",buf,buf);
			if (buf == MSG_TYPE_ACK) {
				espLastcomm = millis();
				return true;
			}
		}
	}
	return false;
} // send_up

// add messages to the queueus
bool connection_relay::enqueue_up(char * msg, uint16_t size){
	uint8_t * buf;

	buf = new uint8_t[size + 3];
	if (buf) {
		buf[0] = size / 256;
		buf[1] = size % 256;
		memcpy(buf + 2, msg, size);
		buf[size + 2] = 0x00; // just to be sure
		for (uint8_t i = 0; i < MAX_MSG_QUEUE; i++) {
			if (!outBuf[i]) {
				// erial.print("Enqueue Slot ");
				// erial.println(i);
				// delay(100);
				outBuf[i] = buf;
				return true;
			}
		}
	}
	logger.println(TOPIC_CON_REL, F("Send queue overflow"), COLOR_RED);
	return false;
}

// check if there are messages to send (previously needed due to async server, now flowcontroll)
// send one message at the time (still fast)
bool connection_relay::dequeue_up(){
	uint32_t start;
	uint16_t size;
	uint8_t buf;

	for (uint8_t i = 0; i < MAX_MSG_QUEUE; i++) {
		if (outBuf[i]) {
			size = outBuf[i][0] * 256 + outBuf[i][1]; // size is stored in the first two byte, MSB First
			// send up
			if (send_up((char *) outBuf[i] + 2, size)) {
				delete outBuf[i];
				outBuf[i] = 0x00;
				// erial.println("send done");
				return true;
			} else {
				logger.println(TOPIC_CON_REL, F("Send up failed"), COLOR_RED);
				return false;
			}
		}
	}
	return true;
}

// start ap, set IP to "same as main connection +1" master 192.168.N.1 -> AP 192.168.(N+1).1
// create and start wifiserver on port 1883
void connection_relay::startAP(){
	if (!m_AP_running) {
		logger.println(TOPIC_CON_REL, F("Starting AP"));
		WiFi.mode(WIFI_AP_STA);
		IPAddress apSM = IPAddress(255, 255, 255, 0);
		IPAddress ip   = WiFi.localIP();
		ip[2] = ip[2] + 1; // increase 3rd octed for each sublevel
		ip[3] = 1;
		char ssid[strlen(AP_SSID)+4+strlen(mqtt.dev_short)]; // max 32?
		sprintf_P(ssid,PSTR("%s_%s_L%i"),AP_SSID,mqtt.dev_short,m_mesh_level+1);
		sprintf_P(m_msg_buffer, PSTR("Set AP IP to %i.%i.%i.%i and SSID to %s"), ip[0], ip[1], ip[2], ip[3], ssid);
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);
		WiFi.softAPConfig(ip, ip, apSM);
		WiFi.softAP(ssid, AP_PW, WiFi.channel(), 0); // 1 == hidden SSID
		m_AP_running = true;

		espServer = new WiFiServer(MESH_PORT);
		espServer->begin();

		logger.println(TOPIC_CON_REL, F("AP started"), COLOR_GREEN);
	}
}

// stop access point and stop server
void connection_relay::stopAP(){
	if (m_AP_running) {
		logger.println(TOPIC_CON_REL, F("Stopping AP"));
		if (espServer) {
			espServer->stop();
			delete espServer;
			espServer = NULL;
		}
		WiFi.softAPdisconnect(true);
		WiFi.mode(WIFI_STA);
		m_AP_running = false;
		logger.println(TOPIC_CON_REL, F("AP stopped"));
	}
}

// this will be called from the main loop
// 1. if we are a mesh node, check if our uplink has info for us. parse it and run callback, which will consume the data or call publish
// 2. check if our server has new incoming connections and handle them
// 3. check all connected clients if new data are available and call the parser if needed
// 4. add keep alives to queue if needed
// 5. send data upstream if there are any in the queue
// 6. if we are directly connected to the MQTT, loop our mqtt client
// 7. disconnect dead mesh clients
void connection_relay::receive_loop(){
	if (m_connection_type == CONNECTION_MESH_CONNECTED) {
		// handle data from uplink (function 1)
		uint16_t size = espUplink.available();
		while (size) {
			uint8_t buf[2];
			sprintf_P(m_msg_buffer, PSTR("Received %i byte from Uplink"), size);
			logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);
			espUplink.readBytes(buf, 1);
			if (buf[0] == MSG_TYPE_PUBLISH) {
				// get length for topic and message from the buffer
				uint8_t topic_len;
				uint16_t msg_len;
				espUplink.readBytes(&topic_len, 1);
				espUplink.readBytes(buf, 2);
				msg_len = (buf[0] << 8) | buf[1];
				// Serial.printf("received msg with %i byte \r\n",msg_len);
				// this is a bit hacky .. we could receive very long messages, (OTA is 950 byte)
				// that would force us to allocate quite a bit of memory, as of now there is no checking in place
				// if that worked out or not, but currently (20180227 we're running at 28k free RAM)
				char msg[msg_len + 1];
				char topic[topic_len + 1];
				// fill buffer .. again dangerous ?
				espUplink.readBytes(topic, topic_len); // topic is NOT \0 terminated in the message
				topic[topic_len] = '\0';
				if (espUplink.readBytes(msg, msg_len + 1) == msg_len + 1) {
					// msg[msg_len]     = '\0';
					callback(topic, (byte *) msg, msg_len);
					// there is one more byte in the buffer, after the msg and that is the \0 for the msg
				} else {
					logger.println(TOPIC_CON_REL, F("Can get all data from buffer"), COLOR_RED);
				}
			}
			size = espUplink.available();
		}
		// handle keepalives (function 4)
		if ((millis() - espLastcomm) / 1000 > COMM_TIMEOUT / 2) { // 70 sec
			char buf[2];
			buf[0] = MSG_TYPE_PING;
			buf[1] = 0x00;
			enqueue_up(buf, 1);
		}
		// espLastcomm will be received when we receive a MSG_TYPE_ACK from our uplink
		// MSG_TYPE_ACK will be send to a subnode whenever we receive data
		// if we haven't received a response in time .. disconnect
		if((millis()-espLastcomm) / 1000 > COMM_TIMEOUT ){
			sprintf_P(m_msg_buffer, PSTR("Connection to server timed out. Disconnecting"));
			logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_RED);
			disconnectServer();
		}
		// send messages upstream (function 5)
		dequeue_up();
	} else if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
		// handle data from MQTT link if any (function 6)
		client.loop();
	}
	// new client (function 2)
	if (espServer != NULL) {
		if (espServer->hasClient()) {
			WiFiClient * c = new WiFiClient(espServer->available());
			onClient(c);
		}
	}
	// incoming traffic (function 3) and handle disconnects (function 7)
	for (int i = 0; i < ESP8266_NUM_CLIENTS; i++) {
		if (espClients[i] != NULL) {
			if (espClients[i]->connected()) {
				if (espClients[i]->available()) {
					espClientsLastComm[i] = millis();
					onData(espClients[i], i);
				} else if (espClientsLastComm[i] > 0 && (millis() - espClientsLastComm[i]) / 1000 > COMM_TIMEOUT * 1.2) { // 1.2 to be able to miss one and have 20% headroom
					sprintf_P(m_msg_buffer, PSTR("Client %i timed out. Disconnecting"), i);
					logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_RED);
					espClients[i]->stop();
					delete espClients[i];
					espClients[i] = NULL;
				}
			} else {
				sprintf_P(m_msg_buffer, PSTR("Client %i disconnected"), i);
				logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);
				delete espClients[i];
				espClients[i] = NULL;
			}
		}
	}
} // receive_loop

// will be callen by the receive_loop if the server has new espClients
// purpose is to link the client to the pointer structure
void connection_relay::onClient(WiFiClient * c){
	sprintf_P(m_msg_buffer, PSTR("(%s) Incoming client connection"), c->remoteIP().toString().c_str());
	logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_PURPLE);
	for (int i = 0; i < ESP8266_NUM_CLIENTS; i++) {
		if (espClients[i] == NULL) {
			sprintf_P(m_msg_buffer, PSTR("Assign id: %i"), i);
			logger.println(TOPIC_CON_REL, m_msg_buffer);

			espClients[i] = c;
			char buf[2];
			buf[0] = MSG_TYPE_ACK;
			buf[1] = 0x00;
			c->write(buf, 1);

			espClientsLastComm[i] = millis();
			break;
		}
	}
}

// will be callen by the receive_loop if the client has data available
// should do the parsing and enqueue_up the response
void connection_relay::onData(WiFiClient * c, uint8_t client_nr){
	size_t len = c->available();
	uint8_t data[len];

	c->read(data, len);
	// sprintf_P(m_msg_buffer,"Got data from %s",c->remoteIP().toString().c_str());
	// logger.println(TOPIC_CON_REL, m_msg_buffer,COLOR_YELLOW);
	char msg_ack[2];
	msg_ack[0] = MSG_TYPE_ACK;
	msg_ack[1] = '\0';
	c->write(msg_ack, 1);
	// erial.println(msg);
	if (((char *) data)[0] == MSG_TYPE_SUBSCRPTION) {
		// [0] type
		// [1..] topic
		uint8_t * topic_start = ((uint8_t *) data) + 1;
		sprintf_P(m_msg_buffer, PSTR("[%i](%s) FWD subscription '%s'"), client_nr, c->remoteIP().toString().c_str(), topic_start);
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);

		subscribe((char *) topic_start, true); // subscribe via enqueue
	} else if (((char *) data)[0] == MSG_TYPE_PUBLISH) {
		// [0] type
		// [1] length of topic
		// [2] length of msg
		uint8_t * topic_start = ((uint8_t *) data) + 3;
		uint8_t * msg_start   = ((uint8_t *) data) + ((uint8_t *) data)[1] + 3;

		// make routing more obvious, replace generic mesh AP SSID with parent node name in SSID msg from client
		if (strstr((char *) topic_start,
		   build_topic("SSID", UNIT_TO_PC, false)) != NULL && strcmp((char *) AP_SSID, (char *) msg_start) == 0) {
			logger.println(TOPIC_CON_REL, F("Replacing client SSID with my name"), COLOR_PURPLE);
			msg_start = (uint8_t *) mqtt.dev_short;
		}

		sprintf_P(m_msg_buffer, PSTR("[%i](%s) FWD '%s' -> '%s'"), client_nr,
		  c->remoteIP().toString().c_str(), topic_start, msg_start);
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);

		publish((char *) topic_start, (char *) msg_start, true); // publish via enqueue
	} else if (((char *) data)[0] == MSG_TYPE_NW_LOOP_CHECK) {
		if (m_connection_type == CONNECTION_MESH_CONNECTED) { //  don't care it if we're then head, no loop here
			// copy message to send it up the chain
			char msg[len + 1];
			memcpy(msg, (char *) data, len);
			msg[len] = '\0';

			// prepare our own message to compare both
			char msg_check[20]; // 1 + 2*6 + 5
			uint8_t mac[6];
			WiFi.macAddress(mac);
			sprintf_P(msg_check, PSTR("%c%02x:%02x:%02x:%02x:%02x:%02x"), MSG_TYPE_NW_LOOP_CHECK, mac[5], mac[4], mac[3], mac[2], mac[1],
			  mac[0]);

			sprintf_P(m_msg_buffer, PSTR("[%i](%s) NETLOOP check '%s' vs '%s'"), client_nr,
			  c->remoteIP().toString().c_str(), msg + 1, msg_check + 1);
			logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_PURPLE);

			// do the check
			if (strcmp(msg, msg_check) == 0) {
				// fuck .. loop .. disconnect network
				logger.println(TOPIC_CON_REL, F(
				   "Received own MAC, loop detected! Blacklisting server and disconnecting!"), COLOR_RED);
				bl->set_fails(WiFi.BSSID(), 1); // increase blacklist counter by one for this wifi
				disconnectServer();
			} else {
				logger.println(TOPIC_CON_REL, F("Not my MAC, send on it."), COLOR_GREEN);
				enqueue_up(msg, len + 1);
			}
		} // no else needed, if we're the direct connected node then this showsn that there is no loop
	} else if (((char *) data)[0] == MSG_TYPE_ROUTING) {
		// preparation
		if (strlen(mqtt.dev_short) > 6) { // to make sure
			mqtt.dev_short[6] = 0x00;
		}
		// [0] type
		// [1] length of topic
		// [2] length of msg
		// [3..] topic, which will end with 0x00
		// [..] message, which will end with 0x00
		char msg[len + 5 + strlen(mqtt.dev_short)];                             // what ever is in it + my name + " -> '\0'"
		memcpy(msg, data, len);                                                 // copy existing message, this includes th \0
		memcpy(msg + len - 1, " >> ", 4);                                       // -1 to replace \0
		memcpy(msg + len - 1 + 4, &mqtt.dev_short, strlen(mqtt.dev_short) + 1); // copy name + \0

		uint8_t * topic_start = ((uint8_t *) msg) + 3;
		uint8_t * msg_start   = ((uint8_t *) msg) + ((uint8_t *) msg)[1] + 3;
		logger.println(TOPIC_CON_REL, F("Adding my name to routing chain"), COLOR_PURPLE);
		sprintf_P(m_msg_buffer, PSTR("[%i](%s) FWD routing '%s' -> '%s'"), client_nr,
		  c->remoteIP().toString().c_str(), topic_start, msg_start);
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);

		if (m_connection_type == CONNECTION_DIRECT_CONNECTED) {
			publish((char *) topic_start, (char *) msg_start);
		} else {
			enqueue_up(msg, len + 4 + strlen(mqtt.dev_short)); // len-1+4+strlen()+1
		}
	} else if (((char *) data)[0] == MSG_TYPE_PING) {
		sprintf_P(m_msg_buffer, PSTR("[%i](%s) Received keep alive"), client_nr, c->remoteIP().toString().c_str());
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_GREEN);
	} else {
		sprintf_P(m_msg_buffer,PSTR("[%i](%s) Received unsupported message type '%i'"), client_nr, c->remoteIP().toString().c_str(),
		  ((char *) data)[0]);
		logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_RED);
	}
} // onData

// funcion to handle updates via MQTT
// not completed
uint8_t connection_relay::mqtt_ota(uint8_t * data, uint16_t size){
	// [0] CMD
	// [1] if CMD = WRITE: length
	// [2] data
	// ////// start of OTA ////////
	if (data[0] == MQTT_OTA_BEGIN) {
		m_ota_total_update_size = atoi((char *) (data + 1));
		m_ota_bytes_in = 0;
		m_ota_seq      = 0;
		uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
		sprintf_P(m_msg_buffer, PSTR("Total: %i, %i available"), m_ota_total_update_size, maxSketchSpace);
		if (maxSketchSpace > m_ota_total_update_size) {
			if (!Update.begin(m_ota_total_update_size)) { // start with max available size
				logger.println(TOPIC_OTA, F("Begin failed"), COLOR_RED);
				Update.printError(Serial);
				m_ota_status = OTA_STATUS_START_FAILED;
			} else {
				logger.println(TOPIC_OTA, m_msg_buffer, COLOR_GREEN);
				m_ota_status = OTA_STATUS_OK;
			}
		} else {
			logger.println(TOPIC_OTA, m_msg_buffer, COLOR_RED);
			m_ota_status = OTA_STATUS_NOT_ENOUGH_SPACE;
		}
		return m_ota_status;
	}
	// ////// incoming OTA data ////////
	else if (data[0] == MQTT_OTA_WRITE) {
		// some sort of seq check
		if (m_ota_status != OTA_STATUS_OK) {
			return m_ota_status;
		}
		m_ota_bytes_in += size - 3;
		uint16_t seq = ((uint16_t) data[1]) << 8 | data[2];
		if (seq == m_ota_seq) {
			logger.resetSerialLine(); // erase the[mqtt in] line
			uint16_t p = ((uint32_t) m_ota_bytes_in * 1000) / m_ota_total_update_size;
			sprintf_P(m_msg_buffer, PSTR("%02x%02x%02x.. %2i.%01i%% of %i byte"), data[3], data[4], data[5], p / 10, p % 10,
			  m_ota_total_update_size);
			logger.println(TOPIC_OTA, m_msg_buffer, COLOR_GREEN);

			sprintf_P(m_msg_buffer, PSTR(" %2i.%01i%%"), p / 10, p % 10);
			network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);

			if (Update.write(data + 3, size - 3) != size - 3) {
				logger.println(TOPIC_OTA, F("Write failed"), COLOR_RED);
				Update.printError(Serial);
				m_ota_status = OTA_STATUS_WRITE_FAILED;
			}
			m_ota_seq++;
		} else {
			m_ota_status = OTA_STATUS_SEQ_FAILED;
			logger.println(TOPIC_OTA, F("Seq failed"), COLOR_RED);

			sprintf_P(m_msg_buffer, PSTR("update failed 3"));
			network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);
		}
		return m_ota_status;
	}
	// ////// end of OTA transmission ////////
	else if (data[0] == MQTT_OTA_END) {
		if (m_ota_status != OTA_STATUS_OK) {
			sprintf_P(m_msg_buffer, PSTR("update failed 1"));
			network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);

			return m_ota_status;
		}
		;
		Update.setMD5((char *) (data + 1));
		logger.resetSerialLine(); // erase the[mqtt in] line
		if (Update.end(true)) {   // true for whatever reason, will reboot the new sketch
			logger.println(TOPIC_OTA, F("Update Success\r\nRebooting..."), COLOR_PURPLE);

			sprintf_P(m_msg_buffer, PSTR("update success"));
			network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);

			delay(1000);
			ESP.restart();
			return 0;
		} else {
			logger.println(TOPIC_OTA, F("Update failed..."), COLOR_RED);
			Update.printError(Serial);

			sprintf_P(m_msg_buffer, PSTR("update failed 2"));
			network.publish(build_topic("INFO", UNIT_TO_PC), m_msg_buffer);

			m_ota_status = OTA_STATUS_END_FAILED;
			return m_ota_status;
		}
	}

	// ////// catch ////////
	m_ota_status = OTA_STATUS_UNKNOWN_CMD;
	return m_ota_status;
} // mqtt_ota

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// a blacklist is a self organized data structure that can store and retrieve information about unsuccessfl connects to a BSSID
blacklist_entry::blacklist_entry(uint8_t * MAC){
	m_next = NULL;
	memcpy(m_mac, MAC, 6);
	m_fails       = 0;
	m_last_change = millis() / 1000;
};

blacklist_entry::~blacklist_entry(){ };


// get the number of failed connection attempts to the passed MAC
// if this node has a different MAC it will relay the request to the next node and return its response
// if there are no further nodes but the MAC is still not found: Create a new node with the MAC in doubt
// and return the init value 0
uint8_t blacklist_entry::get_fails(uint8_t * MAC){
	if (strncmp((char *) m_mac, (char *) MAC, 6) == 0) {
		// sprintf_P(m_msg_buffer, "Blacklist for this WiFi is %i", m_fails);
		// logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_YELLOW);
		if (m_fails > 0 && ((millis() / 1000) - m_last_change > BLACKLIST_RECOVER_TIME_SEC)) { // if node was unable to connect more then 6 hours ago: give it another chance ..
			m_fails--;
			m_last_change = millis() / 1000;
		}
		return m_fails;
	}
	if (m_next == NULL) {
		blacklist_entry * new_entry = new blacklist_entry(MAC);
		if (!new_entry) {
			return 255; // creation failed
		}
		m_next = new_entry;
	}
	return m_next->get_fails(MAC);
}

// same as above, but this time SET the fail counter. v=0 will reset the value, evenything else will increase it by one
// setting it to 2 and above with disqualiify the node from being connected
// Overtime this value will be decreaesed, so node which have a bad rating can recover slowly
void blacklist_entry::set_fails(uint8_t * MAC, uint8_t v){
	if (strncmp((char *) m_mac, (char *) MAC, 6) == 0) { // this is me
		if (v == 0) {
			m_fails = 0;
		} else {
			m_fails++;
			sprintf_P(m_msg_buffer, PSTR("Setting blacklist for current BSSID to %i"), m_fails);
			logger.println(TOPIC_CON_REL, m_msg_buffer, COLOR_RED);
		}
		m_last_change = millis() / 1000;
	} else { // not me, ask next
		if (m_next == NULL) {
			get_fails(MAC);
		}
		return m_next->set_fails(MAC, v);
	}
}

// ////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////
