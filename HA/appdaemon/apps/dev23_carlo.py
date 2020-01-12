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
        self.listen_state(self.main_toggle,"binary_sensor.dev23_button1s", new = "on")
        self.listen_state(self.lampinions_toggle,"binary_sensor.dev23_button", new = "on")

    def lampinions_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle lampinion")
        if(self.get_state("input_boolean.carlo_switch_lock")=="off"):
           self.toggle("light.dev56")
        else:
           self.log("carlo lock active")

    def main_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle main")
        if(self.get_state("input_boolean.carlo_switch_lock")=="off"):
           self.toggle("light.dev23")
        else:
           self.log("carlo lock active")
