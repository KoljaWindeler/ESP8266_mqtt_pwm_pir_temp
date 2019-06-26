import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class PiWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Pi Service")
        self.listen_state(self.fuse, "sensor.dev18_update")
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
        try:
            g=int(self.get_state("sensor.dev8_update"))
            b=int(self.get_state("sensor.dev18_update"))
        except:
            g=0
            b=0

        if(b==8):
            m="Also der Brunnen meldet sich seit "+str(b)+" minuten nicht mehr "
            if(g>6):
                m=m+"und die Garage ist auch schon ueber "+str(g)+" min offline, ich glaube es ist der FI raus"
            else:
                m=m+"aber die Garage hat sich zuletzt vor "+str(g)+" minuten gemeldet, schein also nicht die Sicherung zu sein."
            self.log(m)
            self.call_service("notify/pb", title="Connection loss", message=m)
            self.call_service("notify/pb_c", title="Connection loss", message=m)
            self.log("send")
