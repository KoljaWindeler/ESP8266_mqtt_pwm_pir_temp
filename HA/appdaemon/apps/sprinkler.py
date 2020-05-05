import appdaemon.plugins.hass.hassapi as hass
import datetime, time

#
# Hellow World App
#
# Args:
#

SWITCH_PUMP = "switch.dev4_gpio_12"
SWITCH_POWER = "switch.dev30_gpio_2"
SWITCH_VALVE1 = "switch.dev30_gpio_16"
SWITCH_VALVE2 = "switch.dev30_gpio_4"
SWITCH_VALVE3 = "switch.dev30_gpio_5"
SWITCH_VALVE4 = "switch.dev30_gpio_15"
SWITCH_VALVE5 = "switch.dev30_gpio_14"

class SprinklerWorld(hass.Hass):
	def g(self,entity,default):
		try:
			r = self.get_state(entity)
			if(not(isinstance(r,str))):
				self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
				r = default
			if(r=="unknown"):
				self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
				r = default
		except:
			r = default
			self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
		return r

	def check_stop(self):
		if(self.g(SWITCH_PUMP,"off")=="off" or self.g("input_boolean.irrigation","off")=="off"): 
			return True
		else:
			return False

	def initialize(self):
		self.log("Starting Sprinkler Service")
		self.listen_state(self.start,"input_boolean.irrigation", new = "on")
		self.listen_state(self.start_o,"input_boolean.irrigation_override", new = "on")
		self.turn_off(SWITCH_PUMP)
		self.run_daily(self.start_o, datetime.time(8, 0, 0))


	def start_o(self, entity="", attribute="", old="", new="",kwargs=""):
		# set on, if we have been started by irrigation
		self.turn_on("input_boolean.irrigation")

	def start(self, entity, attribute, old, new,kwargs):
		#self.call_service("notify/pb", title="Irrigation", message="Starting")
		self.log("################################################")
		self.log("Starting Sprinker call")
		self.set_state("sensor.dev30_state",state="Starting")
		self.max_time = 15 #30 # Minutes!

		self.limited_config = [[[SWITCH_VALVE5],1]]

		self.config = [
			[[SWITCH_VALVE4,SWITCH_VALVE5],1], # one valve, 2xt200 + vegtables, regular time
			[[SWITCH_VALVE2],1], # one valve, 3xt200, regular time
			[[SWITCH_VALVE1,SWITCH_VALVE3],1.5] # two valves, 150% time
				]

		self.valve_list = [SWITCH_VALVE1,SWITCH_VALVE2,SWITCH_VALVE3,SWITCH_VALVE4,SWITCH_VALVE5]

		#today_max_temp = int(float(self.g("sensor.yweather_temperature_max","28")))
		if(self.g("input_boolean.irrigation_override","off")=="off"):
			today_max_temp = int(float(self.g("sensor.dark_sky_daytime_high_temperature_0d","28")))
			if(today_max_temp > 28):
				self.max_time = 30
			elif(today_max_temp < 22):
				if(today_max_temp > 5):
					# leave the time at 15 min, but only run limted config (only veggetables)
					self.log("running limited config only")
					self.config = self.limited_config
			self.log("Today max temp is "+str(today_max_temp)+", will irrigate for "+str(self.max_time)+ " min")
			rain_today = int(float(self.g("sensor.dev30_uptime","0"))/5)
		else:
			self.turn_off("input_boolean.irrigation_override")
			rain_today = 0
			self.log("irrigation override, setting time to fix "+str(self.max_time)+" and ignore rain time")

		self.max_time = max(self.max_time - rain_today,0)
		self.log("There was already "+str(rain_today)+" min of rain, so I'll irrigate for "+str(self.max_time)+" now")

		if(self.max_time>0):
			self.state = 100 # Prepare pump, substep 0
			self.sm() # start statemachine
		else:
			self.state = 4000 # Prepare pump, substep 0
			self.sm() # start statemachine


		
	def sm(self, entity="", attribute="", old="", new="",kwargs=""):
		if(int(self.state/100) == 1): # [01xx] prepare pumps
			###### valve online?
			if(self.state%100 == 0): # [01xx] & [xx00] = [0100]
				self.log("################################################")
				self.log("Preparing Pump")
				self.set_state("sensor.dev30_state",state="Preparing pump")
				self.log("Activating valve power")
				# try 50x 6sec to turn of the values (very rarly they are offline right when we need them)
				if(int(self.g("sensor.dev30_update","0"))>0):
					#self.call_service("notify/pb", title="Irrigation", message="Valves offline, waiting up to 5 min")
					self.set_state("sensor.dev30_state",state="Valve offline, wait 0/50")
					self.state = 101
					self.run_in(self.sm,6) # call in again in 6 sec
					return
				else:
					self.set_state("sensor.dev30_state",state="Valve online")
					self.state = 160
			elif(self.state%100 <= 50): # # [01xx] & [xx<=50] = 0101 - 0150: wait cycles for valve to come online
				if(int(self.g("sensor.dev30_update","0"))>0):
					if(self.state%100 < 50):
						self.set_state("sensor.dev30_state",state="Valve offline, wait "+str(self.state%100)+"/50")
						self.state += 1
						self.run_in(self.sm,6) # call in again in 6 sec
						return
					else:
						self.set_state("sensor.dev30_state",state="Valve offline, give up")
						self.state = 0
				else:
					self.set_state("sensor.dev30_state",state="Valve online")
					self.state = 160
			###### valve online !
			if(self.state%100 == 60): # [01xx] & [xx60] = [0160]
				## start of pump startup cycle
				self.turn_on(SWITCH_POWER)
				self.turn_on(SWITCH_PUMP)
				time.sleep(2) # give a little time to start the pump, else check_stop will stop us imidiatly
				self.log("Opening all valves to reduce pump pressure")
				self.set_state("sensor.dev30_state",state="Open all valves")
				for sprinkler in range(0,len(self.valve_list)):
					self.log("Valve "+self.valve_list[sprinkler]+" open")
					self.turn_on(self.valve_list[sprinkler])
				
				self.state = 161
			
			###### pump starting
			if(int(self.state/10) >= 16): # # [01xx] & [x>=160] = 160-199
				round = int(self.state/10)-16 # 160->0, 170->1, 180->2
				on_off = self.state%2 # 161->1->on, 162->0->off
				sec = ((self.state%10)-1)*10 # 161 = (1-1)*10 = 0 // 162 = (2-1)*10 = 10
				
				self.set_state("sensor.dev30_state",state="Ramp up "+str(round+1)+"/3, "+str(sec)+"/40")
				self.log("Ramp up "+str(round+1)+"/3, "+str(sec)+"/40")
				if(on_off == 1):
					self.turn_on(SWITCH_PUMP)
				else:
					# it should be on, but it is off .. hmm 
					if(self.check_stop()):
						print("ohohO 222")
						print(self.g(SWITCH_PUMP,"off"))

						self.state = 4000 # finish up
						self.run_in(self.sm,1) # call in again in 1 sec to finish up
						return

					self.turn_off(SWITCH_PUMP)
				
				if(self.state%10==4): # 164 => round 0, on_off 0(off), sec 30
					if(int(self.state/10)==18): # 160,170, 180
						self.state = 200 # run irigation
					else: # continues with ramp up
						self.state = int(self.state/10)*10+11 # 171 => round 1, on_off 1 on, sec 0
				else:
					self.state += 1 # same round, toggle on_off, inc sec by 10
				
				self.run_in(self.sm,10) # call in again in 10 sec
				return
			
		########## finish preparation
		if(self.state == 200): # [0200]
			self.turn_on(SWITCH_PUMP)
			self.log("Pump ready")
			time.sleep(2) # give a little time to g states
			self.log("Closing all valves")
			for sprinkler in range(0,len(self.valve_list)):
				self.turn_off(self.valve_list[sprinkler])
				self.log("Valve "+self.valve_list[sprinkler]+" closed")
			self.log("Preperation completed, starting the irrigation")
			self.log("################################################")
			self.set_state("sensor.dev30_state",state="Preparation done")
			self.state = 3000
		
		########## start of irigation
		if(int(self.state/1000) == 3): # [3xxx]
			ring = int(self.state/100)%10 # 3[ring][minute (2 digits)] = 3000 -> ring 0, 0 min on // 3010 -> ring 0, 10 min on
			min = self.state%100
			if(not(self.check_stop())):
				# calc time D = time in minues that this ring should run
				D = self.max_time * self.config[ring][1]
				if(min==0):
					# turn values on
					self.log("Turn ring "+str(ring+1)+" on.")
					for sprinkler in range(0,len(self.config[ring][0])):
						self.turn_on(self.config[ring][0][sprinkler])
					self.set_state("sensor.dev30_state",state="Ring "+str(ring+1)+" -> "+str(D-min)+" min")
					self.state += 1
					self.run_in(self.sm,60) # call in again in 60 sec
					return
				else:
					# we're already sprinkling
					self.log("Currently running ring "+str(ring+1)+" for further "+str(D-min)+" min")
					self.set_state("sensor.dev30_state",state="Ring "+str(ring+1)+" -> "+str(D-min)+" min")
					if(self.check_stop()):
						self.state = 0
						self.call_service("notify/pb", title="Irrigation", message="Pump off, stopping")
						self.log("Pump off, stopping irrigation")
						self.set_state("sensor.dev30_state",state="Stopping")
						self.state = 4000 # finish up
						self.run_in(self.sm,1) # call in again in 1 sec to finish up
						return
					else:
						# advance state
						if(D-min<=0):
							# this was the last minute, close ring
							self.log("Turn ring "+str(ring+1)+" off.")
							for sprinkler in range(0,len(self.config[ring][0])):
								self.turn_off(self.config[ring][0][sprinkler])

							# was this the last ring?
							if(ring+1 == len(self.config)):
								# yes, all done, finish up
								self.state = 4000
							else:
								#no, next ring
								self.state = 3000+(ring+1)*100
								self.run_in(self.sm,1) # call in again in 1 sec
								return
						else:
							# keep running
							self.state += 1
							self.run_in(self.sm,60) # call in again in 60 sec
							return
			else:
				# check stop hit true: shut down
				self.log("Skipping ring "+str(ring+1)+" pump is off")
				self.state = 4000 # finish up
		
		########## finish up
		if(int(self.state/1000) == 4):
		
			# shut power down
			self.set_state("sensor.dev30_state",state="Shutting down")
			self.log("Shutting down valve power")
			self.turn_off(SWITCH_POWER)
			self.log("Shutting down pump")
			self.turn_off(SWITCH_PUMP)

			# shut down all valves
			for sprinkler in range(0,len(self.valve_list)):
				self.turn_off(self.valve_list[sprinkler])
				self.log("Valve "+self.valve_list[sprinkler]+" closed")

			# toggle rain time reset
			self.turn_on("switch.dev30_reset_rain_time")
			time.sleep(1)
			self.turn_off("switch.dev30_reset_rain_time")

			# finish
			self.log("All done")
			self.turn_off("input_boolean.irrigation")
			self.log("All done, enjoy your evening")
			self.set_state("sensor.dev30_state",state="Done")
			self.log("################################################")
			return
						
						
						
						
						
						
						
