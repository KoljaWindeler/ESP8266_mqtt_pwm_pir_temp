import appdaemon.plugins.hass.hassapi as hass
import datetime, time


class motionLight(hass.Hass):

	def initialize(self):
		self.log("Starting Motion Service")
		self.my_sensors = self.args["sensors"]
		self.my_lights = self.args["lights"]
		for i in self.my_sensors:
			#self.log("Sensor: "+i)
			self.listen_state(self.motion,i)
		self.turn_light_off()

	def turn_light_off(self, entity="", attribute="", old="", new="",kwargs=""):
		for i in self.my_lights:
			if(self.get_state(i) == "on"):
				self.turn_off(i)
		try:
			self.cancel_timer(self.handle)
		except:
			pass

	def turn_light_on(self, entity="", attribute="", old="", new="",kwargs=""):
		for i in self.my_lights:
			if(self.get_state(i) == "off"):
				self.turn_on(i)

	def motion(self, entity="", attribute="", old="", new="off",kwargs=""):
		#self.log("Toggle motion")
		# start with cleanup,
		# either motion is on, so light should be on as well
		if(new == "on"):
			self.turn_light_on()
			# or it just turned off, so we should schedule a timer and stop the last
			try:
				self.cancel_timer(self.handle)
			except:
				pass
		# so if motion is off, check if all sensors are in the off state now
		if(new == "off"):
			time.sleep(1) # sleep 1sec, sometimes we get ON->OFF withing 50ms. Since off will run faster, on will cancle the timer that we set in off. avoid this by running this slower
			all_off = True
			for i in self.my_sensors:
				if(self.get_state(i) == "on"):
					all_off = False
					break
			if(all_off):
				self.handle = self.run_in(self.turn_light_off,int(self.args["delay"])*60)
