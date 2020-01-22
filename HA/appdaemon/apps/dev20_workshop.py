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
        self.listen_state(self.group_toggle,"light.dev20")
        self.listen_state(self.vent_toggle,"binary_sensor.dev20_button1s", new="on")

    def group_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle group lights")
        self.set_state("light.joiner_workshop",state=new)
        if(new=="off"):
           self.turn_off("light.dev28")
        else:
           self.turn_on("light.dev28")
        self.set_state("binary_sensor.dev59_motion_13",state="on")
        time.sleep(1)
        self.set_state("binary_sensor.dev59_motion_13",state="off")


    def vent_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle ventilation")
        self.toggle("switch.dev28_gpio_4") # bench only
