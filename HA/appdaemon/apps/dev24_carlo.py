import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class carloWorld(hass.Hass):

    def initialize(self):
        self.log("Starting carlo Service")
        self.set_state("sensor.dev24_0_hold",state="0")
        self.listen_state(self.main_toggle,"sensor.dev24_0_hold", new = "1")
        self.listen_state(self.lampinions_toggle,"light.dev24_2", new = "on")

    def lampinions_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle lampinion")
        self.toggle("light.dev55")

    def main_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle main")
        self.set_state("sensor.dev24_0_hold",state="0")
        self.toggle("light.dev24") # main
