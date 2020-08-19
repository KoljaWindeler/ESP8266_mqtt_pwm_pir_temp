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
		self.dbg = True
		self.update()
		runtime = datetime.time(23, 50, 0)
		self.run_daily(self.update, runtime)

		############ smart power ###############
		self.dbg_sp = 0

		self.sp_safe_budget = 250
		self.sp_est_pwr = [] # estimated power
		self.sp_remaining = [] # min thst the device shall run
		self.sp_dev = []
		self.sp_timesensor = []
		self.sp_constrain = []
		self.sp_constrain_l = []
		self.sp_constrain_u = []

		self.sp_add(dev="switch.dev40_gpio_12",pwr=240, time=60*24,timesensor="dev40_drytime")
		self.sp_add(dev="switch.dev30_gpio_0", pwr=600, time=180*99,timesensor="dev30_pumptime")
		self.sp_add(dev="switch.dev38_gpio_5", pwr=2200,time=60*24,timesensor="dev38_heattime")

		self.run_daily(self.sp_reset, runtime)
		self.sp_time = datetime.datetime.now()
		self.listen_state(self.sp, "sensor.dev37_em_cur_fast")
		############ smart power ###############



	def sp_add(self,dev,pwr,time,timesensor,constrain="",lower=0,upper=0):
		self.sp_est_pwr.append(pwr) # estimated power
		self.sp_remaining.append(time) # min thst the device shall run
		self.sp_dev.append(dev)
		self.sp_timesensor.append(timesensor)
		self.sp_constrain.append(constrain)
		self.sp_constrain_l.append(lower)
		self.sp_constrain_u.append(upper)

	def sp_reset(self, kwargs=""):
		self.log("resetting smart power runtime")
		for i in range(0,len(self.sp_dev)):
			self.call_service("mqtt/publish",topic=self.sp_timesensor[i], payload = "0", qos = "2", retain="true")

	def sp(self, entity="", attribute="", old="", new="", kwargs=""):
		try:
			net = float(self.get_state("sensor.dev37_em_cur_fast"))
			solar = float(self.get_state("sensor.kaco_194"))
		except:
			self.log("smart power exception")
			return
		elapsedTime = (datetime.datetime.now() - self.sp_time).total_seconds()
		self.sp_time = datetime.datetime.now()


		### log
		msg = "Smart power, solar: "+str(solar)+", net: "+str(net)
		if(net>0):
			msg += " so we're drawing power from the net"
		elif(net<2*(-self.sp_safe_budget)):
			msg += " so we're feeding the net above safety margin"
		else:
			msg += " so we're feeding the net but haven't even reached safety margin"
		if(self.dbg_sp):
			self.log(msg)

		### switch on in order ###
		for d in range(0,len(self.sp_dev)):
			rem = float(self.get_state("sensor."+self.sp_timesensor[d]))
			if(self.get_state(self.sp_dev[d]) == "off"):
				#self.log("Device "+self.sp_dev[d]+" is off, ran "+str(rem)+" / "+str(self.sp_remaining[d]))
				if(rem < self.sp_remaining[d]):
					if(net*-1 > self.sp_est_pwr[d]+self.sp_safe_budget*2): # only switch on if we would have still extra
						if(self.sp_constrain[d]!=""):
							v = float(self.get_state(self.sp_constrain[d]))
							if(self.dbg_sp):
								self.log("there is a contrain on "+str(self.sp_dev[d])+" current value: "+str(v)+" and limits["+str(self.sp_constrain_l[d])+","+str(self.sp_constrain_u[d])+"]")
							if(v>=self.sp_constrain_l[d] and v<=self.sp_constrain_u[d]):
								self.log("[TURN ON] "+self.friendly_name(self.sp_dev[d]))
								self.turn_on(self.sp_dev[d])
								return # only switch on one at the time
						else:
							self.log("[TURN ON] "+self.friendly_name(self.sp_dev[d]))
							self.turn_on(self.sp_dev[d])
							return # only switch on one at the time
		### switch on in order ###
		### switch off in invert order ###
		for i in range(0,len(self.sp_dev)):
			d = len(self.sp_dev) - i - 1
			rem = float(self.get_state("sensor."+self.sp_timesensor[d]))
			if(self.get_state(self.sp_dev[d]) == "on"):
				rem = round(rem+elapsedTime/60,3) # runs every 15 sec, so add a quarter sec
				self.call_service("mqtt/publish",topic=self.sp_timesensor[d], payload = str(rem), qos = "2", retain="true")

				#self.log("Device "+self.sp_dev[d]+" is on, ran "+str(rem)+" / "+str(self.sp_remaining[d]))
				if(rem >= self.sp_remaining[d]):
					self.turn_off(self.sp_dev[d])
					self.log("[time out, turn off] "+self.friendly_name(self.sp_dev[d]))
					return
				elif(net > -self.sp_safe_budget): # switch off if we have less then 500W budget
					self.turn_off(self.sp_dev[d])
					self.log("[power out, turn off] "+self.friendly_name(self.sp_dev[d]))
					return
				else:
					if(self.sp_constrain[d]!=""):
						v = float(self.get_state(self.sp_constrain[d]))
						if(v<self.sp_constrain_l[d] or v>self.sp_constrain_u[d]):
							self.turn_off(self.sp_dev[d])
							self.log("[constrain, turn off] "+self.friendly_name(self.sp_dev[d]))
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

		net = float(self.get_state("sensor.dev37_em_stst",attribute="total"))
		net = net/(60*24*1000)
		kaco = float(self.get_state("sensor.kaco_194_kwh"))
		selfconsume = kaco+net
		self.log("generiert: "+str(kaco)+", verbraucht: "+str(selfconsume)+", eingespeist: "+str(-net))

