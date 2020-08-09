import math
import appdaemon.plugins.hass.hassapi as hass
import datetime
import wait

class EnergyWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Energy Service")
		wait.wait_available(self,"sensor.dev37_em_tot",False)
		#self.listen_state(self.update, "input_boolean.clear_Energy")
		self.cost_kwh = 0.2231
		self.cost_fix_month = 17.85
		self.payment_month = 115

		self.sp_safe_buget = 250
		self.sp_est_pwr = [600,2200] # estimated power
		self.sp_remaining = [3*60,9999] # min thst the device shall run
		self.sp_dev = ["switch.dev30_gpio_0","switch.dev38_gpio_5"]
		self.sp_timesensor = ["dev30_pumptime","dev38_heattime"]

		self.dbg = True
		self.update()
		runtime = datetime.time(23, 50, 0)
		self.run_daily(self.update, runtime)
		self.run_daily(self.sp_reset, runtime)

		time = datetime.datetime.now() + datetime.timedelta(seconds=10)
		self.run_every(self.sp, time, 15) # every 15 sec


	def sp_reset(self, kwargs=""):
		self.log("resetting smart power runtime")
		for i in range(0,len(self.sp_dev)):
			self.call_service("mqtt/publish",topic=self.sp_timesensor[i], payload = "0", qos = "2", retain="true")

	def sp(self, kwargs=""):
		pwr = float(self.get_state("sensor.dev37_em_cur"))
		self.log("Smart power, current power "+str(pwr))
		### switch on in invert order ###
		for d in range(0,len(self.sp_dev)):
			rem = float(self.get_state("sensor."+self.sp_timesensor[d]))
			if(self.get_state(self.sp_dev[d]) == "off"):
				self.log("Device "+self.sp_dev[d]+" is off, ran "+str(rem)+" / "+str(self.sp_remaining[d]))
				if(rem < self.sp_remaining[d]):
					if(pwr*-1 > self.sp_est_pwr[d]+self.sp_safe_buget*2): # only switch on if we would have still 1kW extra
						self.log("turn on "+self.sp_dev[d])
						self.turn_on(self.sp_dev[d])
						return
		### switch on in invert order ###
		### switch off in invert order ###
		for i in range(0,len(self.sp_dev)):
			d = len(self.sp_dev) - i - 1
			rem = float(self.get_state("sensor."+self.sp_timesensor[d]))
			if(self.get_state(self.sp_dev[d]) == "on"):
				rem = rem + 0.25 # runs every 15 sec, so add a quarter sec
				self.call_service("mqtt/publish",topic=self.sp_timesensor[d], payload = str(rem), qos = "2", retain="true")

				self.log("Device "+self.sp_dev[d]+" is on, ran "+str(rem)+" / "+str(self.sp_remaining[d]))
				if(rem >= self.sp_remaining[d]):
					self.turn_off(self.sp_dev[d])
					self.log("[time out] turn off "+self.sp_dev[d])
					return
				elif(pwr > -self.sp_safe_buget): # switch off if we have less then 500W budget
					self.turn_off(self.sp_dev[d])
					self.log("[power out] turn off "+self.sp_dev[d])
					return
		### switch off in invert order ###


	def update(self, entity="", attribute="", old="", new="", kwargs=""):
		if(not(isinstance(new,str)) or not(isinstance(old,str))):
			self.log("not a string")
			return

		now = datetime.date.today()
		e_now = float(self.get_state("sensor.dev37_em_tot"))
		e_old = 0
		c = self.get_state("sensor.long_term_power",attribute="all")['attributes']['entries']
		for i in c:
			p = datetime.datetime.strptime(i.split(',')[0], "%Y-%M-%d")
			if(p.year == now.year):
				e_old = float(i.split(',')[1])
				break
		if(e_old):
			days = (now-p.date()).days + 1
			e_con = e_now - e_old
			fix = self.cost_fix_month * 12 / 365 * days
			usage = self.cost_kwh * e_con
			cost = fix + usage
			payment = self.payment_month * 12 / 365 * days
			total = payment - cost

			try:
				self.set_state("sensor.energy_cost",state=str(total))
			except:
				self.log("could not set cost")

			if(self.dbg):
				self.log("using startdate: "+str(p)+" that is "+str(days)+" days ago")
				self.log("Used energy "+str(int(e_con))+" kWh = "+str(int(e_now))+" - "+str(int(e_old)))
				self.log("total: "+str(int(total*100)/100)+" eur = "+str(int(payment*100)/100)+" - "+str(int(cost*100)/100)+" (fix:"+str(int(fix*100)/100)+", usage:"+str(int(usage*100)/100)+")")
