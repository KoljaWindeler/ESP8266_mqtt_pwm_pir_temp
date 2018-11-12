/**
 * This is a kind of unit test for DEV for now
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <time.h>

#define TRIGGER_PIN 0
const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };

WiFiManager wm;
bool TEST_CP  = false; // always start the configportal
bool TEST_NET = true; // do a network test, get ntp time
char ssid[] = "*************";  //  your network SSID (name)
char pass[] = "********";       // your network password

void saveWifiCallback(){
  Serial.println("[CALLBACK] saveCallback fired");
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("[CALLBACK] configModeCallback fired");
  // myWiFiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); 
  // Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  // Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");
}

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);
  // wm.server->on("/info",handleRoute); // can override wm!
}

void handleRoute(){
  Serial.println("[HTTP] handle route");
  wm.server->send(200, "text/plain", "hello from user code");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  // delay(3000);
  Serial.println("\n Starting");
  // WiFi.setSleepMode(WIFI_NONE_SLEEP); // disable sleep, can improve ap stability
  
  wm.debugPlatformInfo();

  //Local intialization. Once its business is done, there is no need to keep it around
  //reset settings - for testing
  // wm.resetSettings();
  // wm.erase();
  
  wm.setClass("invert");

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  // wm.setConfigPortalTimeout(600);
  // wm.setConnectTimeout(5);
  // wm.setShowStaticFields(true);

  WiFiManagerParameter custom_html("<p>This Is Custom HTML</p>"); // only custom html
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", "", 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", "", 6);
  WiFiManagerParameter custom_token("api_token", "api token", "", 16);
  WiFiManagerParameter custom_tokenb("invalid token", "invalid token", "", 0); // id is invalid, cannot contain spaces
  WiFiManagerParameter custom_ipaddress("input_ip", "input IP", "", 15,"pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'"); // custom input attrs (ip mask)

  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);

  //add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_token);
  wm.addParameter(&custom_tokenb);
  wm.addParameter(&custom_ipaddress);

  custom_html.setValue("test",4);
  custom_token.setValue("test",4);

  // Set cutom menu via menu[] or vector
  // const char* menu[] = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
  // wm.setMenu(menu,9); // custom menu array must provide length

  std::vector<const char *> menu = {"wifi","wifinoscan","info","param","close","sep","erase","restart","exit"};
  // wm.setMenu(menu); // custom menu, pass vector
  
  // set static sta ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // wm.setShowStaticFields(false);
  // wm.setShowDnsFields(false);

  // set static ip
  // wm.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  // wm.setAPStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); 

  // WiFi.mode(WIFI_STA);
  // const wifi_country_t COUNTRY_US{"US",1,11,WIFI_COUNTRY_POLICY_AUTO};
  // const wifi_country_t COUNTRY_CN{"CN",1,13,WIFI_COUNTRY_POLICY_AUTO};
  // const wifi_country_t COUNTRY_JP{"JP",1,14,WIFI_COUNTRY_POLICY_AUTO};
  // esp_wifi_set_country(&COUNTRY_US);

  // wm.setCountry("US");
  
  wm.setConfigPortalTimeout(120);
  // wm.startConfigPortal("AutoConnectAP", "password");

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  
  if(!wm.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
  }
  else if(TEST_CP) {
    // start configportal always
    wm.setConfigPortalTimeout(60);
    wm.startConfigPortal();
  }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
  }
  pinMode(TRIGGER_PIN, INPUT);
}

void loop() {
  // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    Serial.println("BUTTON PRESSED");
    wm.setConfigPortalTimeout(120);

    // disable captive portal redirection
    // wm.setCaptivePortalEnable(false);
    
    if (!wm.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
      getTime();
    }
  }

  if(WiFi.status() == WL_CONNECTED)  getTime();
  // put your main code here, to run repeatedly:
  delay(5000);
}

void getTime() {
  int tz           = -5;
  int dst          = 0;
  time_t now       = time(nullptr);
  unsigned timeout = 5000;
  unsigned start   = millis();  
  configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2 ) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
    if((millis() - start) > timeout){
      Serial.println("[ERROR] Failed to get NTP time.");
      return;
    }  
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void debugchipid(){
  // WiFi.mode(WIFI_STA);
  // WiFi.printDiag(Serial);
  // Serial.println(modes[WiFi.getMode()]);
  
  // ESP.eraseConfig();
  // wm.resetSettings();
  // wm.erase(true);
  WiFi.mode(WIFI_AP);
  // WiFi.softAP();
  WiFi.enableAP(true);
  delay(500);
  // esp_wifi_start();
  delay(1000);
  WiFi.printDiag(Serial);
  delay(60000);
  ESP.restart();

  // AP esp_267751
  // 507726A4AE30
  // ESP32 Chip ID = 507726A4AE30
}