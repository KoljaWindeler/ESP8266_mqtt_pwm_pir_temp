import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class PiWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Pi Service ")

        self.run_daily(self.off, time(23, 0, 0))
        self.run_at_sunset(self.on, offset = -15 * 60)

    ######################################################

    def on(self, entity="", attribute="", old="", new="",kwargs=""):
        self.log("Pi on")
        self.turn_on("light.dev18")

    def off(self, entity="", attribute="", old="", new="",kwargs=""):
        self.log("Pi off")
        self.turn_off("light.dev18")
