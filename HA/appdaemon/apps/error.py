import math
import appdaemon.plugins.hass.hassapi as hass


class ErrorWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Error Service")
		self.listen_state(self.update, "input_boolean.clear_error")


	def update(self, entity="", attribute="", old="", new="", kwargs=""):
		self.log("update")
		#self.log(self.sensors)
		self.set_state("sensor.error",state=" ")
		self.set_state("input_boolean.clear_error",state="off")

