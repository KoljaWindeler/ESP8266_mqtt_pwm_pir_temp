import appdaemon.plugins.hass.hassapi as hass
import datetime, time,random

#
# Hellow World App
#
# Args: GartenWasserWashinge
#

class WashingWorld(hass.Hass):
	def initialize(self):
		self.log("Starting Washing Service")
		self.state = 0

		self.thr_power = 100
		self.thr_min = 10

		self.wash_ts = datetime.datetime.now().time()
		self.listen_state(self.update, "sensor.dev16_ads_ch0_kw")

		self.handle_m = []
		self.handle_t = []

	def update(self, entity='', attribute='', old='', new='',kwargs=''):
		#self.log("updated power to: "+new+" current state is "+str(self.state))
		if(self.state < 2):
			#self.log("state <2")
			if(float(new) >= self.thr_power):
				self.log("high power, inc state")
				self.state += 1
			if(self.state == 2):
				self.log("state == 2, setting time")
				self.wash_ts = datetime.datetime.now()
		elif(self.state == 2):
			if(float(new) >= self.thr_power):
				self.log("high power, washing")
				self.wash_ts = datetime.datetime.now()
			elif(self.wash_ts + datetime.timedelta(minutes=self.thr_min) < datetime.datetime.now()):
				self.log("low power, for 10 minutes, i guess we're done")
				self.state = 3

				self.handle_m.clear()
				self.handle_m.append(self.listen_state(self.motion, "binary_sensor.dev59_motion_13"))
				self.handle_m.append(self.listen_state(self.motion, "binary_sensor.dev59_motion_16"))
				self.handle_m.append(self.listen_state(self.motion, "binary_sensor.dev54_motion_2"))

				t="Washing machine"
				m="Hi, my work is done :) your beloved washing machine"
				self.call_service("notify/pb", title=t, message=m)
				self.call_service("notify/pb_c", title=t, message=m)
				self.handle_t.clear()
				self.handle_t.append(self.run_in(self.chk,30*60))
				self.handle_t.append(self.run_in(self.chk,60*60))
				self.handle_t.append(self.run_in(self.chk,90*60))
				self.handle_t.append(self.run_in(self.chk,120*60))
			else:
				self.log("low power, but during 10 min backoff time")

	def motion(self, entity='', attribute='', old='', new='',kwargs=''):
		self.log("motion: "+entity+" new="+new+" state "+str(self.state))
		if(self.state>=3):
			if(new=="on"):
				try:
					for i in self.handle_t:
						self.cancel_timer(i)
					for a in self.handle_m:
						self.cancel_listen_state(a)
				except:
					pass
				self.state = 0

	def chk(self, entity='', attribute='', old='', new='',kwargs=''):
		self.log("chk, state is "+str(self.state))
		t="Washing machine"
		if(self.state==3): #30
			m="Hello ??"
			self.state = 4
		elif(self.state==4): #60
			m="i mean ... seriously ??"
			self.state = 5
		elif(self.state==5): #90
			m="we need to talk"
			self.state = 6
		elif(self.state==6): #120
			m="honey, I miss you"
			self.state = 0
			for a in self.handle_m:
				self.cancel_listen_state(a)
		self.call_service("notify/pb", title=t, message=m)
		self.call_service("notify/pb_c", title=t, message=m)
