import math
import appdaemon.plugins.hass.hassapi as hass
import datetime, time
from threading import Timer


class LastUpdatedWorld(hass.Hass):

	def initialize(self):
		self.log("Starting LastUpdated Service")
		prefix = ["sensor","binary_sensor","device_tracker"]
		all = self.get_state("sensor")
		self.sensors = []
		for i in all:
			if(i[-2:]=="_t"):
				#self.log("searching parent for: "+i)
				name = i[:-2]
				name = name.split(".")[1]
				#self.log("-"+name+"-")
				for p in prefix:
					parent = p+"."+name
					if(self.entity_exists(parent)):
						lookUp = [parent,i]
						self.sensors.append(lookUp)
						self.listen_state(self.update, parent)
						#self.log("parent "+parent+" found")

		self.timing_handle = self.timer()


	def parse(self,input):
		#self.log(input)
		ts = self.convert_utc(self.get_state(input,attribute="all")["last_changed"]).timestamp()
		ts = math.floor((datetime.datetime.now().timestamp() - ts)/60)
		if(ts==0):
			return "now"
		elif(ts<60):
			return str(ts)+" min"
		elif(ts<60*24):
			return str(math.floor(ts/60))+" h"
		else:
			return str(math.floor(ts/1440))+" d"

	def timer(self, entity="", attribute="", old="", new="", kwargs=""):
		try:
			self.cancel_timer(self.timing_handle)
		except:
			pass
		self.timing_handle = self.run_in(self.timer,delay=60)
		self.update(kwargs="timer")

	def update(self, entity="", attribute="", old="", new="", kwargs=""):
		#self.log("update "+kwargs)
		#self.log(self.sensors)
		for s in self.sensors:
			#self.log("Updating "+s[1]+" with value "+self.parse(s[0]))
			self.set_state(s[1],state=self.parse(s[0]))


