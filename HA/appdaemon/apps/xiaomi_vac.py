import appdaemon.plugins.hass.hassapi as hass
import datetime, time
import wait 

#
# Hellow World App
#
# Args:
#

class xiaomi_vacWorld(hass.Hass):

	def initialize(self):
		self.log("Starting xiaomi_vac Service")
		self.run_daily(self.reset, datetime.time(0, 0, 0))
		self.mct = 20*60 # minimum clean time 20min
		if(wait.wait_available(self,["binary_sensor.someone_is_home","vacuum.xiaomi_vacuum_cleaner","vacuum.xiaomi_vacuum_cleaner_2"],False)):
			self.listen_state(self.presents,"binary_sensor.someone_is_home") # instant
			self.vacs = ["vacuum.xiaomi_vacuum_cleaner","vacuum.xiaomi_vacuum_cleaner_2"]
			self.vac_clean_interval = [2,1]
			self.cleaning_started = []
			self.tct = [] # total cleaning time
			self.is_cleaning = [] # status

			for vac in self.vacs:
				self.listen_state(self.cleaning,vac)
				self.cleaning_started.append(time.time())
				self.tct.append(0)
				self.is_cleaning.append(False)
			self.reset()

	def g_tct(self,id):
		if(self.is_cleaning[id]):
			return self.tct[id] + time.time() - self.cleaning_started[id]
		else:
			return self.tct[id]


	def cleaning(self, entity="", attribute="", old="", new="",kwargs=""):
		if(not(new==old)):
			try:
				self.log("new vacuum ("+str(entity)+") status: "+new+". old was "+old+". tct: "+str(self.g_tct(self.vacs.index(entity))))
				if(new=="cleaning"):
					self.is_cleaning[self.vacs.index(entity)] = True
					self.cleaning_started[self.vacs.index(entity)] = time.time()
				elif(self.is_cleaning[self.vacs.index(entity)]):
					self.is_cleaning[self.vacs.index(entity)] = False
					self.tct[self.vacs.index(entity)] += time.time() - self.cleaning_started[self.vacs.index(entity)]
					self.log("cleaning stopped, total time: "+str(self.g_tct(self.vacs.index(entity))))
					if(self.tct[self.vacs.index(entity)] > self.mct):
						self.set_state("input_boolean.cleaning_done_today_"+str(self.vacs.index(entity)),state="on")
			except:
				pass

	def presents(self, entity, attribute, old, new,kwargs):
		self.log("VAC ... "+str(old)+" -> "+str(new))
		if(new=="off"):
			try:
				self.cancel_timer(self.handle)
				self.log("vacuum timer stopped")
			except:
				pass
			self.handle = self.run_in(self.vacuum_start,300)
			self.log("vacuum timer set")
		elif(new=="on"):
			try:
				self.cancel_timer(self.handle)
				self.log("vacuum timer stopped")
			except:
				pass
			self.vacuum_stop()


	def vacuum_stop(self, kwargs=""):
		self.log("someone is home, stop vacuuming.")
		for vac in self.vacs:
			self.log("Vac: "+vac+" tct: "+str(self.g_tct(self.vacs.index(vac))))
			self.call_service("vacuum/return_to_base", entity_id=vac)
	def vacuum_start(self, kwargs=""):
		self.log("Home alone.")
		for vac in self.vacs:
			self.log("Vac: "+vac+" tct: "+str(self.g_tct(self.vacs.index(vac))))
			if(self.tct[self.vacs.index(vac)]>20*60):
				self.log("tct >20 min cleaned already, enough for today")
			elif(self.get_state("input_boolean.autostart_cleaning") == "on"):
				self.log("start cleaning")
				self.call_service("vacuum/start", entity_id=vac)
			else:
				self.log("no cleaning, autostart off")


	def reset(self, entity="", attribute="", old="", new="", kwargs=""):
		for vac in self.vacs:
			if(datetime.datetime.today().weekday() % self.vac_clean_interval[self.vacs.index(vac)]==0):
					self.tct[self.vacs.index(vac)] = 0
					self.set_state("input_boolean.cleaning_done_today_"+str(self.vacs.index(vac)),state="off")

	def wait_available2(self, entity):
		count = 0
		dev = []
		if(isinstance(entity, str)):
			dev.append(entity)
		else:
			dev = entity
		while(count < 300):
			ret = True
			for i in dev:
				if(not(self.entity_exists(i))):
					ret = False
					break
			if(ret):
				self.log("All devices online after "+str(count)+" sec")
				return True
			else:
				count += 1
				time.sleep(1)
		self.log("entities never came online ")
		self.log(dev)
		return False
