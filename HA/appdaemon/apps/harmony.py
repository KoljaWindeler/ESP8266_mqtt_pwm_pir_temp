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
        self.state()

    def state(self, entity="", attribute="", old="", new="",kwargs=""):
        if(not(new=="PowerOff")):
            self.set_state("binary_sensor.harmony",state="on")
            self.log("new harmony state "+new+" -> on")
        else:
            self.set_state("binary_sensor.harmony",state="off")
            self.log("new harmony state "+new+" -> off")



