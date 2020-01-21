import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class temperatureWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Temperature Service")
        all_entities = self.get_state("sensor")
        for e in all_entities:
            if(e.find("_temperature")>0):
                self.listen_state(self.t, e)

    def t(self, entity, attribute, old, new, kwargs):
        #self.log("Sensor "+entity+" reported new value "+new)
        whitelist=["sensor.dev29_temperature"]
        try:
           if(float(new)>50):
               if(entity in whitelist):
                   msg="Whitelisted: "+self.friendly_name(entity)+" ("+entity+") new temperature: "+new
                   #self.log(msg)
               else:
                   msg=self.friendly_name(entity)+" ("+entity+") on fire: "+new
                   self.log(msg)
                   self.call_service("notify/pb", title="FIRE", message=msg)
        except:
           pass

