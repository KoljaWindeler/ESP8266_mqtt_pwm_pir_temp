import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class harmonyWorld(hass.Hass):

    def initialize(self):
        self.log("Starting harmony Service")
        self.remote=["remote.tvhub","remote.beamer_harmony" ]
        self.binary=["harmony","harmony_beamer"]

        for i in range(0,len(self.remote)):
           self.listen_state(self.state,self.remote[i])
           self.state(entity=self.remote[i], new=self.get_state(self.remote[i]))

    def state(self, entity="", attribute="", old="", new="",kwargs=""):
        i=self.remote.index(entity)
        if(not(new=="off")):
            self.set_state("binary_sensor."+self.binary[i],state="on")
            self.log("new "+self.binary[i]+" state "+new+" -> on")
        else:
            self.set_state("binary_sensor."+self.binary[i],state="off")
            self.log("new "+self.binary[i]+" state "+new+" -> off")



