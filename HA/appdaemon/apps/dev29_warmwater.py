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
        self.listen_state(self.fuse, "sensor.dev29_temperature")

    def trigger(self, entity):
        self.log("Toggeling Flow state for 10 sec")
        self.turn_on("switch.dev29")
        time.sleep(10)
        self.turn_off("switch.dev29")
        self.log("all done")

    def fuse(self, entity="", attribute="", old="", new="", kwargs=""):
#        self.log("new boiler temperature "+str(new))
        if(float(new)<35):
            self.call_service("notify/pb", title="Heating", message="Heater at "+str(new)+"c")
            self.call_service("notify/pb_c", title="Heating", message="Heater at "+str(new)+"c")
