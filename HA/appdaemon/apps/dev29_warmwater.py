import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class warm_waterWorld(hass.Hass):

    def initialize(self):
        self.log("Starting warm_water Service")
        self.run_daily(self.trigger, datetime.time(6, 45, 0))
        self.run_daily(self.trigger, datetime.time(6, 15, 0))
        self.run_daily(self.trigger, datetime.time(5, 45, 0))

    def trigger(self, entity):
        self.log("Toggeling Flow state for 10 sec")
        self.turn_on("light.dev29")
        time.sleep(10)
        self.turn_off("light.dev29")
        self.log("all done")
