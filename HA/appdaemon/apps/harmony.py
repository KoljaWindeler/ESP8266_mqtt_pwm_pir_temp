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
        self.listen_state(self.state,"remote.tvhub")
        self.state(new=self.get_state("remote.tvhub"))

    def state(self, entity="", attribute="", old="", new="",kwargs=""):
        if(not(new=="off")):
            self.set_state("binary_sensor.harmony",state="on")
            self.log("new harmony state "+new+" -> on")
        else:
            self.set_state("binary_sensor.harmony",state="off")
            self.log("new harmony state "+new+" -> off")



