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
        self.listen_state(self.presents,"binary_sensor.someone_is_home")
        self.listen_state(self.cleaning,"vacuum.xiaomi_vacuum_cleaner")
        self.cleaning_started = time.time()
        self.tct = 0 # total cleaning time
        self.run_daily(self.reset, datetime.time(0, 0, 0))
        self.mct = 20*60 # minimum clean time 20min
        self.cleaning = False

    def g_tct(self):
        if(self.cleaning):
            return self.tct + time.time() - self.cleaning_started
        else:
            return self.tct

    def cleaning(self, entity, attribute, old, new,kwargs):
        self.log("new vacuum status: "+new+". old was "+old+"+ tct: "+str(self.g_tct()))
        if(not(new==old)):
            if(new=="cleaning"):
                self.cleaning = True
                self.cleaning_started = time.time()
            elif(self.cleaning):
                self.cleaning = False
                self.tct += time.time() - self.cleaning_started
                self.log("cleaning stopped, total time: "+str(self.g_tct()))
                if(self.tct > self.mct):
                    self.set_state("input_boolean.cleaning_done_today",state="on")


    def presents(self, entity, attribute, old, new,kwargs):
        if(new=="on"): #someon is approaching home
            self.log("someone is home, stop vacuuming. tct: "+str(self.g_tct()))
            self.call_service("vacuum/stop", entity_id="vacuum.xiaomi_vacuum_cleaner")
        else:
            self.log("Home alone. tct: "+str(self.g_tct()))
            if(self.tct>20*60):
                self.log(">20 min cleaned already, enough for today")
            else:
                self.log("start cleaning")
                self.call_service("vacuum/start", entity_id="vacuum.xiaomi_vacuum_cleaner")

    def reset(self, entity="", attribute="", old="", new="", kwargs=""):
        self.tct = 0
        self.set_state("input_boolean.cleaning_done_today",state="off")



