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
        self.listen_state(self.mpc,"light.dev32", new="on")

    def aw(self, entity="", attribute="", old="", new="",kwargs=""):
       if(self.get_state("input_boolean.approx_warn")=="on"):
           self.turn_on("switch.dev25_gpio_4") # bench only
           time.sleep(1)
           self.turn_off("switch.dev25_gpio_4") # bench only

    def mpc(self, entity="", attribute="", old="", new="",kwargs=""):
        now = datetime.datetime.now().time()
        if(self.get_state("switch.dev58_gpio_12")=="on"):
#        if(self.get_state("binary_sensor.ping_binary_sensor")=="on"):
            if(now >= datetime.time(22,00,00)):
               t="media pc on"
               m="your bedlight just turn on after 10 and the media pc is still running"
               self.call_service("notify/pb", title=t, message=m)


    def led_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle led lights")
        self.toggle("light.dev10") # led


    def main_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle main lights")
#        self.set_state("light.joiner_workshop",state=new)
        self.toggle("switch.dev25_gpio_4") # bench only
