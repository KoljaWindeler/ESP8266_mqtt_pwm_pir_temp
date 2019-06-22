import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class workshopWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Workshop Service")
        self.set_state("sensor.dev20_button_hold",state="0")
        self.listen_state(self.group_toggle,"light.dev20")
        self.listen_state(self.main_toggle,"sensor.dev20_button_hold")

    def group_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle group lights")
        self.set_state("light.joiner_workshop",state=new)
        if(new=="off"):
           self.turn_off("light.dev19")
        else:
           self.turn_on("light.dev19")
        self.set_state("binary_sensor.dev6_motion",state="on")
        time.sleep(1)
        self.set_state("binary_sensor.dev6_motion",state="off")


    def main_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle main lights")
        self.set_state("sensor.dev20_button_hold",state="0")
        self.toggle("light.dev20") # bench only
