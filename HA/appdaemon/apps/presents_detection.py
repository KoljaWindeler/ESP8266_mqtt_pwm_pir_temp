import appdaemon.plugins.hass.hassapi as hass

class PresentsWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Presents Service")
		self.white_list = ["light.dev12","light.dev12_2","light.dev15","light.dev54_3","light.dev54_4","light.joiner_outdoor","light.dev56","light.dev18"]
		self.listen_state(self.presents, "binary_sensor.someone_is_home") # 5 min away
		self.run_in(self.chk_light,3*60) # delay 3 min so states can settle

	def presents(self, entity, attribute, old, new,kwargs):
		if(old=="on" and new=="off"):
			#m="nobody at home, checking in 5min"
			#self.log(m)
			self.run_in(self.chk_light,5*60)
			self.run_in(self.chk_door,2*60)
			#self.call_service("notify/pb", title="testing", message=m)

	def chk_door(self, kwargs=""):
		if(self.get_state("binary_sensor.someone_is_home") == "off"):
			self.log("Presents state just changed to 'nobody is home'")
			if(self.get_state("binary_sensor.dev17_motion") == "on"):
				m = "everyone is gone for 2 minutes but the basement door is still open"
				t = "This might interest you"
				self.call_service("notify/pb", title=t, message=m)
				self.call_service("notify/pb", title=t, message=m)
				self.log(m)

	def chk_light(self, kwargs=""):
		if(self.get_state("binary_sensor.someone_is_home") == "off" and self.get_state("input_boolean.presents_simulation") == "off"):
			self.log("Presents state just changed to 'nobody is home'")
			# get all device that are on
			remaining_lights_on = []
			group = self.get_state("light")
			for l in group:
				if(self.get_state(l)=="on" and not(l in self.white_list)):
					remaining_lights_on.append(l)
			# if there are some, write a message
			if(len(remaining_lights_on)>0):
				m = "Hi, it seams like nobody is home now for over a minute but there is still light: "
				for l in remaining_lights_on:
					m += self.friendly_name(l)+" ("+l+"), "
				m = m[0:len(m)-2]+" so you might want to tunnel in and turn it off remotly" # remove tailing ", "
				self.log(m)
				self.call_service("notify/pb", title="you can leave your LED on", message=m)
