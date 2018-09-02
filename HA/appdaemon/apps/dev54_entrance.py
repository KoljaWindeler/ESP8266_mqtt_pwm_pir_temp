import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time


#
# Hellow World App
#
# Args:
#

class EntranceWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Entrance Service")
        self.listen_state(self.inside_off, "binary_sensor.dev10_motion", new = "off", duration = 120)
        self.listen_state(self.inside_on, "binary_sensor.dev10_motion", new = "on")

        #self.run_daily(self.outside, time(6, 30, 0))
        #self.run_daily(self.outside, time(22, 0, 0))
        #self.run_daily(self.outside, time(23, 0, 0))
        #self.run_at_sunrise(self.outside, offset = 30 * 60, "Sunrise +30 mins")
        #self.run_at_sunset(self.outside, offset = 15 * 60, "Sunrise +15 mins")
        #self.listen_state(self.inside_on, "binary_sensor.everyone_is_home", new = "on", duration = 10*60)  # everyone is home for 10 min

    def outside(self, entity, attribute, old, new,kwargs):
        now = datetime.now().time()
        #if (now >= time(23,00,00)):
        #    self.turn_off("light.joiner_outdoor")
        #elif(now >= time(22,00,00) and self.get_state("binary_sensor.everyone_is_home") == "on"):
        #    self.turn_off("light.joiner_outdoor")
        #elif(now < time(22,00,00) and self.get_state("binary_sensor.everyone_is_home") == "on"):



    def inside_on(self, entity, attribute, old, new,kwargs):
        now = datetime.now()
        ssm = (now - now.replace(hour=0, minute=0, second=0, microsecond=0)).total_seconds()
        if(ssm<5*60*60 or ssm>22*60*60+30*60):
            self.log("Turn on Chandelair")
            # before 5 or after 22:30, use chandelair
            if(self.get_state("light.dev54")=="on"):
                self.turn_off("light.dev54")
            if(self.get_state("light.dev54_2")=="off"):
                self.turn_on("light.dev54_2")
        else:
            self.log("Turn on LEDs")
            # after 5 and before 22:30, use LED
            if(self.get_state("light.dev54_2")=="on"):
                self.turn_off("light.dev54_2")
            if(self.get_state("light.dev54")=="off"):
                self.turn_on("light.dev54")

    def inside_off(self, entity, attribute, old, new,kwargs):
        self.log("Turn all off")

        if(self.get_state("light.dev54_2")=="on"):
            self.turn_off("light.dev54_2")
        if(self.get_state("light.dev54")=="on"):
            self.turn_off("light.dev54")

