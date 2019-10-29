import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class ventWorld(hass.Hass):

    def initialize(self):
        self.log("Starting vent Service")
        self.listen_state(self.vent, "sensor.dev4_humidity_stat_mean")
        self.turn_off("switch.dev4_gpio_5")
        self.state = 0

    ######################################################

    def vent(self, entity="", attribute="", old="", new="", kwargs=""):
        #self.log("new vent value: "+new+" state "+str(self.state))
        if(float(new)>0.04 and self.state==0):
           self.state = 1
           self.log("switching vent on")
           self.turn_on("switch.dev4_gpio_5")
           self.handle = self.run_in(self.vent_off, seconds=15*60)

    def vent_off(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("switching vent off")
        self.state = 0
        self.turn_off("switch.dev4_gpio_5")
