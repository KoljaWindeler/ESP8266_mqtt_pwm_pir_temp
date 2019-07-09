import appdaemon.plugins.hass.hassapi as hass 
from datetime import datetime, time

class floor_upWorld(hass.Hass):

    def initialize(self):
        self.log("Starting floor_up Service")
        self.listen_state(self.floor_up_off, "binary_sensor.dev57_motion", new = "off", duration = 120)
        self.listen_state(self.floor_up_on, "binary_sensor.dev57_motion", new = "on")
        self.turn_off("light.dev57")

    def floor_up_off(self, entity, attribute, old, new,kwargs):
        #self.log("floor_up off")
        self.turn_off("light.dev57")
#        self.turn_off("light.dev56")
        self.turn_off("light.dev27")

    def floor_up_on(self, entity, attribute, old, new,kwargs):
        now = datetime.now().time()
        b=99
        if (now < time(6,00,00) or now > time(19,00,00)):
            b=11
        #self.log("floor_up on")
        self.turn_on("light.dev57",brightness=b)
#        self.turn_on("light.dev56",brightness=b)
        self.turn_on("light.dev27",brightness=min(255,b*12))
