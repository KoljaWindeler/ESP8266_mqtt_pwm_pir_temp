import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class TestWorld2(hass.Hass):

	def initialize(self):
		self.log("Starting Test Service")
		#self.listen_state(self.six, "binary_sensor.dev54_button", new = "on") #klingel

    ######################################################

	def six(self, entity="", attribute="", old="", new="", kwargs=""):
		self.log("Starting Test Service 2")
