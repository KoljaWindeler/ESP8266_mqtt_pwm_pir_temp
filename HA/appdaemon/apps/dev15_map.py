import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class MapWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Map Service")
        self.listen_state(self.map_off, "binary_sensor.dev15_motion", new = "off", duration = 600)
        self.listen_state(self.map_on, "binary_sensor.dev15_motion", new = "on")

    def map_off(self, entity, attribute, old, new,kwargs):
        #self.log("map off")
        self.turn_off("light.dev15")
        self.turn_off("light.dev12")

    def map_on(self, entity, attribute, old, new,kwargs):
        #self.log("map on")
        self.turn_on("light.dev15")
        ele = float(self.get_state("sun.sun", attribute="elevation"))
        if(ele < 15):
                self.turn_on("light.dev12")
