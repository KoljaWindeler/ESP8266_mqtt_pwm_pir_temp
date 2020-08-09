import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import wait

class PiWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Pi Service")
        wait.wait_available(self,"sensor.dev30_update",False)
        self.listen_state(self.fuse, "sensor.dev30_update")
        self.run_daily(self.off, time(23, 0, 0))
        try:
           self.run_at_sunset(self.on, offset = -15 * 60)
        except:
           pass

    ######################################################

    def on(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("Pi on")
        self.turn_on("light.dev18")

    def off(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("Pi off")
        self.turn_off("light.dev18")

    def fuse(self, entity="", attribute="", old="", new="", kwargs=""):
        try:
            v=int(self.get_state("sensor.dev30_update"))
            b=int(self.get_state("sensor.dev18_update"))
        except:
            v=0
            b=0

        if(v==8):
            m="Also die Ventile melden sich seit "+str(v)+" minuten nicht mehr "
            if(b>6):
                m=m+"und der Brunnen auch schon ueber "+str(b)+" min offline, ich glaube es ist der FI raus"
            else:
                m=m+"aber der Brunnen hat sich zuletzt vor "+str(b)+" minuten gemeldet, schein also nicht die Sicherung zu sein."
            self.log(m)
            self.call_service("notify/pb", title="Connection loss", message=m)
            self.call_service("notify/pb_c", title="Connection loss", message=m)
            self.log("send")
