import appdaemon.plugins.hass.hassapi as hass
import datetime, time,random

#
# Hellow World App
#
# Args: GartenWasserPumpe
#

class PumpWorld(hass.Hass):
	def initialize(self):
		self.log("Starting Pump Service")
		# ch1 = garden, 2 = house
		self.n=["Garden","House"]
		self.s=["sensor.dev16_ads_ch1_kw","sensor.dev16_ads_ch2_kw"]

		self.states = [0,0]
		self.counts = [0,0]
		self.starts = [0,0]
		self.seconds = [0,0]

		self.thr_power = [[600,720],[590,720]]

		self.listen_state(self.update, self.s[0])
		self.listen_state(self.update, self.s[1])
		self.run_daily(self.eval, datetime.time(18, 20, 0))

	def update(self, entity='', attribute='', old='', new='',kwargs=''):
		#self.log("updated power of "+entity+" to: "+new)
		if(not(entity in self.s)):
			return
		e=self.s.index(entity)
		p=int(float(new)-float(old))
		if(self.states[e] == 0 and p >= self.thr_power[e][0] and p <= self.thr_power[e][1]):
			self.states[e] = 1
			self.counts[e] += 1
			self.log("Pump "+self.n[e]+" turned on, count:"+str(self.counts[e]))
			self.starts[e] = datetime.datetime.now()
			self.log("jump by "+str(p)+" Watt")
		elif(self.states[e] == 1 and -p >= self.thr_power[e][0] and -p <= self.thr_power[e][1]):
			self.states[e] = 0
			self.seconds[e] += (datetime.datetime.now()-self.starts[e]).total_seconds()
			self.log("Pump "+self.n[e]+" turned off "+str(int(self.seconds[e]))+" sec")
			self.log("jump by "+str(p)+" Watt")
		elif(p>200 or -p>200):
			self.log("uncatched!!!! jump by "+str(p)+" Watt")


	def eval(self, entity='', attribute='', old='', new='',kwargs=''):
		t="Pumps"
		if(abs(self.counts[0] - self.counts[1])>5 or True):
			m="Hi, irregular pump cycle counts: "+str(self.counts[0])+" / "+str(self.seconds[0])+" at "+self.n[0]+" vs "+str(self.counts[1])+" / "+str(self.seconds[1])+" at "+self.n[1]
			self.call_service("notify/pb", title=t, message=m)
		self.counts[0] = 0
		self.counts[1] = 0
