import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import wait

class ConnectionWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Connection Service")
        wait.wait_available(self,"sensor.dev1_update",True)

        for i in range(1,160):
            if(i==18):
               continue
            e="sensor.dev"+str(i)+"_update"
            if(self.entity_exists(e)):
                self.listen_state(self.t, "sensor.dev"+str(i)+"_update")

    def t(self, entity, attribute, old, new,kwargs):
        if(new=="8"):
            msg=self.friendly_name(entity)+" ("+entity+") new timeout: "+str(new)
            self.log(msg)
            self.call_service("notify/pb", title="connection alert", message=msg)

