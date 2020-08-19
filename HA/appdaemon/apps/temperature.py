import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class temperatureWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Temperature Service")
        all_entities = self.get_state("sensor")
        for e in all_entities:
            if(e.find("_temperature")>0):
                self.listen_state(self.t, e)

        self.pool_kw_thres = 500
        self.pool_kw_timeout = 99
        self.listen_state(self.set_pool,"sensor.dev16_ads_ch3_kw")


    def set_pool(self, entity, attribute, old, new, kwargs):
        #self.log("updateing pool power: "+str(new)+" timeout "+str(self.pool_kw_timeout))
        try:
           if(float(new)>self.pool_kw_thres):
               t = self.get_state("sensor.dev30_temperature")
               #self.log("pool temp: "+str(t))

               if(self.pool_kw_timeout >= 10):
                  self.call_service("mqtt/publish",topic="pool_temp", payload = str(t), qos = "2", retain="true")
                   #self.set_state("sensor.pool_temp",state = t)
               else:
                   self.pool_kw_timeout += 1
           else:
               self.pool_kw_timeout = 0
        except:
           pass

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
                   self.call_service("notify/pb_c", title="FIRE", message=msg)
        except:
           pass

