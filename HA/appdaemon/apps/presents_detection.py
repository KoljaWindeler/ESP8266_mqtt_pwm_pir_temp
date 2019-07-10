import appdaemon.plugins.hass.hassapi as hass

class PresentsWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Presents Service")
		self.listen_state(self.presents, "device_tracker.illuminum_caro", duration = 1*60) # 1 min away 
		self.listen_state(self.presents, "device_tracker.illuminum_kolja", duration = 1*60) # 1 min away


	def presents(self, entity, attribute, old, new,kwargs):
		if(self.get_state("device_tracker.illuminum_caro") == "not_home" and self.get_state("device_tracker.illuminum_kolja") == "not_home"):
			self.log("Presents state just changed to 'nobody is home'")
			# get all device that are on
			remaining_lights_on = []
			group = self.get_state("group.all_lights", attribute = "all")
			for l in group["attributes"]["entity_id"]:
				if(self.get_state(l)=="on"):
					remaining_lights_on.append(l)
			# if there are some, write a message
			if(len(remaining_lights_on)>0):
				m = "Hi, it seams like nobody is home now for over a minute but there is still light: "
				for l in remaining_lights_on:
					m += self.friendly_name(entity)+" ("+entity+"), "
				m = m[0:len(m)-2]+" so you might want to tunnel in an turn it off remotly" # remove tailing ", "
				self.log(msg)
				self.call_service("notify/pb", title="connection alert", message=msg)
