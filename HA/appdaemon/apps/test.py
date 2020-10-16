import appdaemon.plugins.hass.hassapi as hass
import json
from datetime import datetime, time

class TestWorld2(hass.Hass):

	def initialize(self): 
		self.log("Starting Test Service2") 
#		self.call_service("browser_mod/more_info", data = {"deviceID":{"d597bd88_74872538"}, "entity_id": "sensor.kaco_194", "large": "true"})
#		self.call_service("homeassistant/toggle", entity_id="light.d597bd88_74872538")
		
 
    ######################################################

	def six(self, entity="", attribute="", old="", new="", kwargs=""):
		self.log("Starting Test Service 2")
