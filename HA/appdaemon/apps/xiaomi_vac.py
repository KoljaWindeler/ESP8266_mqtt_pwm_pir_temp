import appdaemon.plugins.hass.hassapi as hass
import datetime, time


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
        self.listen_state(self.presents,"binary_sensor.someone_is_home")
        self.vacs = ["vacuum.xiaomi_vacuum_cleaner","vacuum.xiaomi_vacuum_cleaner_2"]
        self.cleaning_started = []
        self.tct = [] # total cleaning time
        self.is_cleaning = [] # status
        
        for vac in self.vacs:
            self.listen_state(self.cleaning,vac)
            self.cleaning_started.append(time.time())
            self.tct.append(0)
            self.is_cleaning.append(False)

    def g_tct(self,id):
        if(self.is_cleaning[id]):
            return self.tct[id] + time.time() - self.cleaning_started[id]
        else:
            return self.tct[id]
        

    def cleaning(self, entity, attribute, old, new,kwargs):
        if(not(new==old)):
            self.log("new vacuum ("+str(entity)+") status: "+new+". old was "+old+". tct: "+str(self.g_tct(self.vac.index(entity))))
            if(new=="cleaning"):
                self.is_cleaning[self.vac.index(entity)] = True
                self.cleaning_started[self.vac.index(entity)] = time.time()
            elif(self.is_cleaning[self.vac.index(entity)]):
                self.is_cleaning[self.vac.index(entity)] = False
                self.tct[self.vac.index(entity)] += time.time() - self.cleaning_started[self.vac.index(entity)]
                self.log("cleaning stopped, total time: "+str(self.g_tct(self.vac.index(entity))))
                if(self.tct[self.vac.index(entity)] > self.mct):
                    self.set_state("input_boolean.cleaning_done_today_"+str([self.vac.index(entity)]),state="on")
                    
    def presents(self, entity, attribute, old, new,kwargs):
        if(new=="on"): #someon is approaching home
            self.log("someone is home, stop vacuuming.")
            for vac in self.vacs:
                self.log("Vac: "+vac+" tct: "+str(self.g_tct(self.vac.index(vac))))
                self.call_service("vacuum/return_to_base", entity_id=vac)
        else:
            self.log("Home alone.")
            for vac in self.vacs:
                self.log("Vac: "+vac+" tct: "+str(self.g_tct(self.vac.index(vac))))
                if(self.tct[self.vac.index(entity)]>20*60):
                    self.log("tct >20 min cleaned already, enough for today")
                elif(self.get_state("input_boolean.autostart_cleaning") == "on"):
                    self.log("start cleaning")
                    self.call_service("vacuum/start", entity_id=vac)
                else:
                    self.log("no cleaning, autostart off")
                

    def reset(self, entity="", attribute="", old="", new="", kwargs=""):
        for vac in self.vacs:
            self.tct[self.vac.index(vac)] = 0
            self.set_state("input_boolean.cleaning_done_today_"+str([self.vac.index(entity)]),state="off")



