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
        self.set_state("sensor.dev23_0_hold",state="0")
        self.listen_state(self.ventilation_toggle,"sensor.dev23_0_hold", new = "1")
        self.listen_state(self.bedlight_toggle,"binary_sensor.dev23_gpio_0", new = "on")

    def bedlight_off(self, entity, attribute, old, new,kwargs):
        self.log("Switch bedlight lights off")
        self.turn_off("light.joiner_bedroom")

    def bedlight_toggle(self, entity, attribute, old, new,kwargs):
        self.log("Toggle bed lights")
        self.toggle("light.joiner_bedroom")

    def ventilation_toggle(self, entity, attribute, old, new,kwargs):
        self.set_state("sensor.dev23_0_hold",state="0")
        self.toggle("switch.dev23_gpio_12") # ventilator
