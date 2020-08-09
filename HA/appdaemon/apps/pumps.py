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
		self.energy = [0,0]

		self.thr_power = [[210,290],[560,720]]

		self.listen_state(self.update, self.s[0])
		self.listen_state(self.update, self.s[1])
		self.run_daily(self.eval, datetime.time(18, 20, 0))

	def update(self, entity='', attribute='', old='', new='',kwargs=''):
		#self.log("updated power or new=='unknown' or old=='unknown' of "+entity+" to: "+new)
		if(not(entity in self.s) or new=='unknown' or old=='unknown'):
			return
		e=self.s.index(entity)
		p=int(float(new)-float(old))
		if(self.states[e] == 0 and p >= self.thr_power[e][0] and p <= self.thr_power[e][1]):
			self.states[e] = 1
			self.counts[e] += 1
			#self.log("Pump "+self.n[e]+" turned on, count:"+str(self.counts[e]))
			self.starts[e] = datetime.datetime.now()
			#self.log("jump by "+str(p)+" Watt")
		elif(self.states[e] == 1 and -p >= self.thr_power[e][0] and -p <= self.thr_power[e][1]):
			self.states[e] = 0
			t = (datetime.datetime.now()-self.starts[e]).total_seconds()
			self.seconds[e] += t
			self.energy[e]+=float(old)/1000*t/3600
			#self.log("Pump "+self.n[e]+" turned off "+str(int(self.seconds[e]))+" sec / "+str(self.energy[e])+" kWh")
			#self.log("jump by "+str(p)+" Watt")
		elif(p>200 or p<-200): # ignore small jumps, everything below 200W
			self.log("uncatched!!!! jump at "+self.n[e]+" by "+str(p)+" Watt")


	def eval(self, entity='', attribute='', old='', new='',kwargs=''):
		t="Pumps"
		if(abs(self.counts[0] - self.counts[1])>5 or True):
			m="Hi, irregular pump cycle counts: "+str(self.counts[0])+" / "+str(int(self.energy[0]*100)/100)+"kWh at "+self.n[0]+"  vs "+str(self.counts[1])+" / "+str(int(self.energy[1]*100)/100)+"kWh at "+self.n[1]
			self.call_service("notify/pb", title=t, message=m)
		self.counts[0] = 0
		self.counts[1] = 0
		self.energy[0] = 0
		self.energy[1] = 0

