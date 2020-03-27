import appdaemon.plugins.hass.hassapi as hass
import datetime, time, random


class motionLight(hass.Hass):

	def initialize(self):
		self.id = random.randrange(0,1000)
		self.dbg = False
		self.log(str(self.id)+" Starting Motion Service")
		self.my_sensors = self.args["sensors"]
		self.my_lights = self.args["lights"]
		for i in self.my_sensors:
			#self.log(str(self.id)+" Sensor: "+i)
			self.listen_state(self.motion,i)
		self.turn_light_off()

	def turn_light_off(self, entity="", attribute="", old="", new="",kwargs=""):
		if(self.dbg):
			self.log(str(self.id)+" timer: turning off")

		for i in self.my_lights:
			if(self.get_state(i) == "on"):
				if(self.dbg):
					self.log(str(self.id)+" actually turning off "+i)
				self.turn_off(i)
		try:
			self.cancel_timer(self.handle)
			if(self.dbg):
				self.log(str(self.id)+" cancel timer worked")
		except:
			if(self.dbg):
				self.log(str(self.id)+" cancel timer failed")
			pass
		self.handle = None

	def turn_light_on(self, entity="", attribute="", old="", new="",kwargs=""):
		for i in self.my_lights:
			if(self.get_state(i) == "off"):
				if(self.dbg):
					self.log(str(self.id)+" actually turning on "+i)
				self.turn_on(i)

	def motion(self, entity="", attribute="", old="", new="off",kwargs=""):
		if(self.dbg):
			self.log(str(self.id)+" Toggle motion "+entity+" to "+new)
		# start with cleanup,
		# either motion is on, so light should be on as well
		try:
			self.cancel_timer(self.handle)
			if(self.dbg):
				self.log(str(self.id)+" cancel timer worked")
		except:
			if(self.dbg):
				self.log(str(self.id)+" cancel timer failed")
			pass
		self.handle = None
		## actuall logic
		if(new == "on"):
			if(self.dbg):
				self.log(str(self.id)+" motion on, light on")
			self.turn_light_on()
			# or it just turned off, so we should schedule a timer and stop the last
		# so if motion is off, check if all sensors are in the off state now
		if(new == "off"):
			time.sleep(1) # sleep 1sec, sometimes we get ON->OFF withing 50ms. Since off will run faster, on will cancle the timer that we set in off. avoid this by running this slower
			all_off = True
			for i in self.my_sensors:
				if(self.get_state(i) == "on"):
					all_off = False
					break
			if(all_off):
				if(self.dbg):
					self.log(str(self.id)+" all off, setting timer")
				self.handle = self.run_in(self.turn_light_off,int(self.args["delay"])*60)
				ttime, interval, kwargs = self.info_timer(self.handle)
				if(self.dbg):
					self.log(ttime)
