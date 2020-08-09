import appdaemon.plugins.hass.hassapi as hass
import datetime, time
import wait

class motionLight(hass.Hass):

	def initialize(self):
		self.log("Starting Motion Service")
		self.my_sensors = self.args["sensors"]
		wait.wait_available(self,self.my_sensors,False)
		
		for i in self.my_sensors:
			#self.log("Sensor: "+i)
			self.listen_state(self.motion,i)
		self.turn_light_off()

	def turn_light_off(self, entity="", attribute="", old="", new="",kwargs=""):
#		self.log("turning off")
		self.turn_off("light.dev57")
		self.turn_off("light.dev27")
		try:
#			self.log("stopping timer after turn off")
			self.cancel_timer(self.handle)
		except:
			pass

	def turn_light_on(self, entity="", attribute="", old="", new="",kwargs=""):
		ele = float(self.get_state("sun.sun", attribute="elevation"))
#        self.log("licht an, elevation is "+str(ele))
		if(ele < 15):
			b=99
			if (self.now_is_between("19:00:00", "06:00:00")):
				b=11
			#self.log("floor_up on")
			self.turn_on("light.dev57",brightness=b)
			self.turn_on("light.dev27",brightness=min(255,b*12))

	def motion(self, entity="", attribute="", old="", new="off",kwargs=""):
		#self.log("Toggle motion")
		# start with cleanup,
		# either motion is on, so light should be on as well
#		self.log("Trigger, "+entity+" state "+new)
		if(new == "on"):
			self.turn_light_on()
			# or it just turned off, so we should schedule a timer and stop the last
			try:
#				self.log("stopping timer after turn ON")
				self.cancel_timer(self.handle)
			except:
				pass
		# so if motion is off, check if all sensors are in the off state now
		if(new == "off"):
			time.sleep(1)
			all_off = True
			for i in self.my_sensors:
				if(self.get_state(i) == "on"):
					all_off = False
					break
			if(all_off):
#				self.log("setting timer to turn off")
				self.handle = self.run_in(self.turn_light_off,int(self.args["delay"])*60)
