import appdaemon.plugins.hass.hassapi as hass
import datetime, time
import wait

#
# Hellow World App
#
# Args:
#

class bedroomWorld(hass.Hass):

    def initialize(self):
        self.log("Starting bedroom Service")
        wait.wait_available(self,["binary_sensor.dev31_button","binary_sensor.dev31_button1s","binary_sensor.dev32_button1s","binary_sensor.dev31_button3s","binary_sensor.dev32_button3s","sensor.dev26_5_hold","sensor.dev26_5_hold","binary_sensor.dev26_gpio_5","light.dev32"],False)

        self.listen_state(self.crip_toggle,"binary_sensor.dev31_button", new = "on")
        self.listen_state(self.bedlight_off,"binary_sensor.dev31_button1s", new = "on")
        self.listen_state(self.bedlight_off,"binary_sensor.dev32_button1s", new = "on")
        self.listen_state(self.bedlight_c_on,"binary_sensor.dev31_button3s", new = "on")
        self.listen_state(self.bedlight_on,"binary_sensor.dev32_button3s", new = "on")
        self.listen_state(self.ventilation_toggle,"sensor.dev26_5_hold", new = "1")
        self.listen_state(self.bedlight_toggle,"binary_sensor.dev26_gpio_5", new = "on")
        self.listen_state(self.mpc,"light.dev32", new="on")

    def bedlight_off(self, entity, attribute, old, new,kwargs):
        self.log("Switch bedlight lights off")
        self.turn_off("light.joiner_bedroom")

    def bedlight_on(self, entity, attribute, old, new,kwargs):
        self.log("Switch bedlight lights on")
        self.turn_on("light.joiner_bedroom")

    def bedlight_c_on(self, entity, attribute, old, new,kwargs):
        self.log("Switch csro lights on")
        self.turn_on("light.dev31")

    def bedlight_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle bed lights")
        self.toggle("light.joiner_bedroom")

    def ventilation_toggle(self, entity, attribute, old, new,kwargs):
        self.toggle("switch.dev26_gpio_15") # ventilator

    def crip_toggle(self, entity, attribute, old, new,kwargs):
        self.toggle("light.dev19") # crip

    def mpc(self, entity="", attribute="", old="", new="",kwargs=""):
        now = datetime.datetime.now().time()
        if(now >= datetime.time(22,00,00)):
            if(self.get_state("switch.dev58_gpio_12")=="on"):
               t="media pc on"
               m="your bedlight just turn on after 10 and the media pc is still running"
               self.call_service("notify/pb", title=t, message=m)
            if(self.get_state("binary_sensor.dev17_motion")=="on"):
               t="Cellar door open"
               m="your bedlight just turn on after 10 and the cellar door is still open!"
               self.call_service("notify/pb", title=t, message=m)
               self.call_service("notify/pb_c", title=t, message=m)

