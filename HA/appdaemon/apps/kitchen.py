import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time 

class kitchenWorld(hass.Hass):

    def initialize(self):
        self.log("Starting kitchen Service")
        self.listen_state(self.propergate, "light.dev3")

    ######################################################

    def propergate(self, entity="", attribute="", old="", new="", kwargs=""):
        if(self.get_state("light.dev3")=="on"):
           self.log("kitchen on")
           self.turn_on("light.dev21")
        else:
           self.log("kitchen off")
           self.turn_off("light.dev21")

