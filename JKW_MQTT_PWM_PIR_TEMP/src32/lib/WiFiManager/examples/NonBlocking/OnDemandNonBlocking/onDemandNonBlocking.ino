/**
 * OnDemandNonBlocking.ino
 * example of running the webportal or configportal manually and non blocking
 * trigger pin will start a webportal for 120 seconds then turn it off.
 * startCP = true will start both the configportal AP and webportal
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

WiFiManager wm;

unsigned int  timeout   = 120; // seconds to run for
unsigned int  startTime = millis();
bool portalRunning      = false;
bool startCP            = false; // start AP and webserver if true, else start only webserver

void setup() {
  // put your setup code here, to run once
  Serial.begin(115200);
  Serial.println("\n Starting");
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
}

void loop() {
  doWiFiManager();
  // put your main code here, to run repeatedly:
}

void doWiFiManager(){
  // is auto timeout portal running
  if(portalRunning){
    wm.process();
    if((millis()-startTime) > (timeout*1000)){
      Serial.println("portaltimeout");
      portalRunning = false;
      if(startCP){
        wm.stopConfigPortal();
      }  
      else{
        wm.stopWebPortal();
      } 
   }
  }

  // is configuration portal requested?
  if(digitalRead(TRIGGER_PIN) == LOW && (!portalRunning)) {
    if(startCP){
      Serial.println("Button Pressed, Starting Config Portal");
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal();
    }  
    else{
      Serial.println("Button Pressed, Starting Web Portal");
      wm.startWebPortal();
    }  
    portalRunning = true;
    startTime = millis();
  }
}


