import math
import appdaemon.plugins.hass.hassapi as hass
import datetime

class EnergyWorld(hass.Hass):

	def initialize(self):
		self.log("Starting Energy Service")
		#self.listen_state(self.update, "input_boolean.clear_Energy")
		self.cost_kwh = 0.2231
		self.cost_fix_month = 17.85
		self.payment_month = 115
		self.dbg = True
		self.update()
		runtime = datetime.time(23, 50, 0)
		self.run_daily(self.update, runtime)

	def update(self, entity="", attribute="", old="", new="", kwargs=""):
		now = datetime.date.today()
		e_now = float(self.get_state("sensor.dev16_em_tot"))
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
