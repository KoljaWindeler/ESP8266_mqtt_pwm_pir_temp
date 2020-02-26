import math
import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
from threading import Timer
from random import *



class SimulationWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Simulation Service")
		self.lights = ["joiner_livingroom","joiner_kitchen","dev24", "joiner_bedroom","dev54_2"]
		self.names = ["Wohnzimmer", "Kueche", "Carlo", "Schlafzimmer","Foyer chandelier"]

		self.run_at_sunset(self.simulation)
		self.simulation()


	def simulation(self, kwargs=""):
		if(not(self.get_state("input_boolean.presents_simulation") == "on")):
			self.log("simulation disabled, quitting")
			return 0

		now = datetime.now().time()
		next = 0

		if(now >= time(22,00,00)):
			self.log("It's late, slow turn off mode")
			# get remaining lights that are still on
			rem = []
			for i in range(0,len(self.lights)):
				if(self.get_state("light."+self.lights[i]) == "on"):
					rem.append("light."+self.lights[i])
			# if there is at least one remainig light on, pick randomly one
			if(len(rem)>0):
				r = randint(0,len(rem)-1)
				self.log(rem)
				self.log("Turn off "+rem[r])
				self.turn_off(rem[r])
			# if there was more than one, rerun in 1-2 min
			if(len(rem)>1):
				next = randint(1,2)
				self.log("Next event in "+str(next)+" min")
		else:
			# its somewhen between sunset and 22:00
			# pick a random light to switch
			# decide if you want the light on or off (0/1)
			r = randint(0,len(self.lights)-1)
			if(randint(0,1)==1):
				self.log("Random on "+self.names[r])
				self.turn_on("light."+self.lights[r])
			else:
				self.log("Random off "+self.names[r])
				self.turn_off("light."+self.lights[r])

			# rerun this
			next = randint(10,20)
			self.log("Next event in "+str(next)+" min")
		# set timer
		if(next>0):
			next*=60 # actually we want to run in minutes, comment this for testing
			self.handle = self.run_in(self.simulation,delay=next)

