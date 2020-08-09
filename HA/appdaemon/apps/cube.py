import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import wait

class cubeWorld(hass.Hass):

	def initialize(self):
		self.log("Starting cube Service")
		# wait.wait_available(self,"sensor.0x00158d0002a7051f_action",False)
		self.listen_state(self.cube, "sensor.0x00158d0002a7051f_action", attribute = "action")

    ######################################################

	def cube(self, entity="", attribute="", old="", new="", kwargs=""):
		c = self.get_state(entity,attribute="all")['attributes']
		self.log(c)
		if(new=="shake"):
			l = "light.joiner_livingroom"
			k = "light.joiner_kitchen"
			if(self.get_state(l)=="on" and self.get_state(k)=="on"):
				self.turn_off(l)
				self.turn_off(k)
			else:
				self.turn_on(l)
				self.turn_on(k)
		elif(new=="flip90"):
			self.toggle("light.dev5")
		elif(new.startswith("rotate")):
			if(new=="rotate_right"):
				self.log("rotate right")
			if(new=="rotate_left"):
				self.log("rotate left")
			angle = min(c['angle'],300)
			add = 0.08/100*angle
			vol = self.get_state("media_player.yamaha_receiver",attribute="all")['attributes']['volume_level']
			self.log("vol level "+str(vol))
			vol = str(vol+add)
			self.call_service("media_player/volume_set", entity_id="media_player.yamaha_receiver", volume_level=vol)

