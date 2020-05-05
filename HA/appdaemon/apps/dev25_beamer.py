import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class beamerWorld(hass.Hass):

    def initialize(self):
        self.log("Starting beamer Service")
        self.listen_state(self.led_toggle,"binary_sensor.dev25_button",new="on")
        self.listen_state(self.main_toggle,"binary_sensor.dev25_button1s", new="on")
        self.listen_state(self.aw,"light.dev59", new="on")

    def aw(self, entity="", attribute="", old="", new="",kwargs=""):
       if(self.get_state("input_boolean.approx_warn")=="on"):
           self.turn_on("switch.dev25_gpio_4") # bench only
           time.sleep(1)
           self.turn_off("switch.dev25_gpio_4") # bench only


    def led_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle led lights")
        self.toggle("light.dev10") # led


    def main_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle main lights")
#        self.set_state("light.joiner_workshop",state=new)
        self.toggle("switch.dev25_gpio_4") # bench only
        # fake motion to turn light on
        self.set_state("binary_sensor.dev59_motion_13",state="on")
        time.sleep(1)
        self.set_state("binary_sensor.dev59_motion_13",state="off")
