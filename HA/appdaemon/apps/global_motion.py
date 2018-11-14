import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class GmotionWorld(hass.Hass):

   def initialize(self):
      self.log("Starting gmotion Service")
      self.sensor = ["6_motion", "8_motion", "10_motion", "14_motion", "15_motion", "54_motion_1", "54_motion_2"]
      for i in range(0,len(self.sensor)):
         self.listen_state(self.m, "binary_sensor.dev"+self.sensor[i])
      self.m("","","","","")

   def m(self, entity, attribute, old, new, kwargs):
      m = False

      for i in range(0,len(self.sensor)):
         #self.log("binary_sensor.dev"+self.sensor[i]+": "+self.get_state("binary_sensor.dev"+self.sensor[i]))
         if(self.get_state("binary_sensor.dev"+self.sensor[i]) == "on"):
            m = True
            break
      if(m):
          self.log("gmotion "+entity)
          self.set_state("binary_sensor.g_motion",state="on")
      else:
          self.log("no gmotion")
          self.set_state("binary_sensor.g_motion",state="off")
