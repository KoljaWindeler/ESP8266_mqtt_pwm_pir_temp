import math
import appdaemon.plugins.hass.hassapi as hass
import datetime, time
from threading import Timer
import wait

class BatteryWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Battery Service")
		wait.wait_available(self,"sensor",True)
		all = self.get_state("sensor")
		self.sensors = []
		self.threshold = 15
		for i in all:
			if(i[-8:]=="_battery"):
				#self.log("found battery device: "+i)
				self.sensors.append(i)
				self.listen_state(self.update, i)
		self.update()


	def update(self, entity="", attribute="", old="", new="", kwargs=""):
		#self.log("update "+kwargs)
		#self.log(self.sensors)
		low_level = False
		m="The following sensors are low on battery:\r\n"
		for s in self.sensors:
			try:
				level = int(self.get_state(s))
				if(level<self.threshold):
					m=m+self.friendly_name(s)+" ("+str(level)+"%)\r\n"
					low_level = True
			except:
				pass
		#self.log("Updating "+s[1]+" with value "+self.parse(s[0]))
		if(low_level):
			self.set_state("sensor.error",state=m)

