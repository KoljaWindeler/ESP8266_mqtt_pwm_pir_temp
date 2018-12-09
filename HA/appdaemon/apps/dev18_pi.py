import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class PiWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Pi Service")
        self.listen_state(self.fuse, "sensor.dev18_info", new = "lost signal")
        self.run_daily(self.off, time(23, 0, 0))
        self.run_at_sunset(self.on, offset = -15 * 60)

    ######################################################

    def on(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("Pi on")
        self.turn_on("light.dev18")

    def off(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("Pi off")
        self.turn_off("light.dev18")

    def fuse(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("signal lost, possible fuse trip")
        self.call_service("notify/pb", title="Connection loss", message="Pi lost connection, fuse tipped?")
        self.call_service("notify/pb_c", title="Connection loss", message="Pi lost connection, fuse tipped?")
        self.log("send")
