import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class bedroomWorld(hass.Hass):

    def initialize(self):
        self.log("Starting bedroom Service")
        self.listen_state(self.bedlight_off,"binary_sensor.dev31_button1s", new = "on")
        self.listen_state(self.bedlight_off,"binary_sensor.dev32_button1s", new = "on")
        self.set_state("sensor.dev26_5_hold",state="0")
        self.listen_state(self.ventilation_toggle,"sensor.dev26_5_hold", new = "1")
        self.listen_state(self.bedlight_toggle,"binary_sensor.dev26_gpio_5", new = "on")

    def bedlight_off(self, entity, attribute, old, new,kwargs):
        self.log("Switch bedlight lights off")
        self.turn_off("light.joiner_bedroom")

    def bedlight_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle bed lights")
        self.toggle("light.joiner_bedroom")

    def ventilation_toggle(self, entity, attribute, old, new,kwargs):
        self.set_state("sensor.dev26_5_hold",state="0")
        self.toggle("switch.dev26_gpio_15") # ventilator
