import appdaemon.plugins.hass.hassapi as hass 
import datetime, time
import urllib.request
import wait

#
# Hellow World App
#
# Args:
#

class AdafruitWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Adafruit Service")
		wait.wait_available(self,"sensor.adafruit",False)
		self.listen_state(self.start,"sensor.adafruit")

	def roundshot(self, url, msg):
		try:
			self.log("downloading "+url)
			fn = "/tmp/ad_tmp.jpg"
			urllib.request.urlretrieve(url, fn)
			self.log("sending")
			self.call_service("notify/pb", data={"file": fn}, message=msg)
		except:
			pass


	def start(self, entity, attribute, old, new,kwargs):
		new = new.lower()
		self.log("New Adafruit input: "+new)

		if(new=="amplifier aus" or new=="tv aus"):
			self.turn_on("script.tvoff")
		if(new=="amplifier an" or new=="tv an"):
			self.turn_on("script.start_chromaudio")
		elif(new=="livingroom an"):
			self.turn_on("light.joiner_livingroom")
		elif(new=="livingroom aus"):
			self.turn_off("light.joiner_livingroom")
		elif(new=="kitchen an"):
			self.turn_on("light.joiner_kitchen")
		elif(new=="kitchen aus"):
			self.turn_off("light.joiner_kitchen")
		elif(new=="spotcleanup"):
			self.call_service("vacuum/clean_spot", entity_id="vacuum.xiaomi_vacuum_cleaner")
		elif(new=="esszimmer"):
			self.turn_on("script.vacuum_kitchen")
		elif(new=="wir_gehen_schlafen"):
			self.turn_off("light.joiner_livingroom")
			self.turn_off("light.joiner_kitchen")
			self.turn_off("light.dev34")
			self.turn_off("light.dev18")
			self.turn_on("script.tvoff")
			self.turn_on("light.dev54_2")
			self.turn_on("light.dev12")
			#self.turn_off("switch.dev4_gpio_12") #pumpe/netz
			self.turn_on("light.dev15")
		elif(new=="garage"):
			self.turn_on("switch.dev8")
		elif(new=="roundshot"):
			self.log("roundshot!")
			self.roundshot("http://192.168.2.11/webcapture.jpg?command=snap&","Roundshot 1/4")
			self.roundshot("http://192.168.2.45/snapshot.jpg","Roundshot 2/4")
			self.roundshot("http://192.168.2.56/snapshot.jpg","Roundshot 3/4")
			self.roundshot("http://192.168.2.42/cgi-bin/currentpic.cgi","Roundshot 4/4")
		elif(new=="0"):
			self.log("ignore 0")
		else:
			self.log(":( no handle found for "+new)
		self.set_state("sensor.adafruit",state="0")


