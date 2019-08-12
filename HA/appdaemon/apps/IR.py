import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class irWorld(hass.Hass):

    def initialize(self):
        self.log("Starting ir Service")
        self.listen_state(self.up,"sensor.dev99_ir")

    def up(self, entity, attribute, old, new,kwargs):
        if(new.find("_done")!=-1):
           return
        #self.log("IR input "+new)
        if(new=="000000000FD6897"):
           self.log("toggle")
           self.toggle("light.dev10")
        self.set_state(entity,state=new+"_done")

