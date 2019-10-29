import appdaemon.plugins.hass.hassapi as hass 
from datetime import datetime, time

class floor_upWorld(hass.Hass):

    def initialize(self):
        self.log("Starting floor_up Service")
        self.listen_state(self.floor_up_off, "binary_sensor.dev57_motion", new = "off", duration = 120)
        self.listen_state(self.floor_up_off, "binary_sensor.dev54_motion_1", new = "off", duration = 120)
        self.listen_state(self.floor_up_on, "binary_sensor.dev57_motion", new = "on")
        self.listen_state(self.floor_up_on, "binary_sensor.dev54_motion_1", new = "on")
        self.turn_off("light.dev57")

    def floor_up_off(self, entity, attribute, old, new,kwargs):
        delta = datetime.now()-self.now
        #self.log(delta.total_seconds())
        if(delta.total_seconds()>120):
           #self.log("floor_up off")
           self.turn_off("light.dev57")
           #self.turn_off("light.dev56")
           self.turn_off("light.dev27")

    def floor_up_on(self, entity, attribute, old, new,kwargs):
        self.now = datetime.now()
        ele = float(self.get_state("sun.sun", attribute="elevation"))
#        self.log("licht an, elevation is "+str(ele))
        if(ele < 15):
           b=99
           if (self.now.time() < time(6,00,00) or self.now.time() > time(19,00,00)):
               b=11
           #self.log("floor_up on")
           self.turn_on("light.dev57",brightness=b)
           #self.turn_on("light.dev56",brightness=b)
           self.turn_on("light.dev27",brightness=min(255,b*12))
