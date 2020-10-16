import appdaemon.plugins.hass.hassapi as hass
import datetime, time
import wait

#
# Hellow World App
#
# Args:
#

class warm_waterWorld(hass.Hass):

    def initialize(self):
        self.log("Starting warm_water Service")
        wait.wait_available(self,["sensor.dev29_temperature","binary_sensor.dev29_gpio_5","sensor.dev29_temperature_der"],False)
        self.hd = []
        self.run_daily(self.on, datetime.time(6, 45, 0))
        self.run_daily(self.on, datetime.time(6, 15, 0))
        self.run_daily(self.on, datetime.time(5, 45, 0))
        self.listen_state(self.fuse, "sensor.dev29_temperature")
        self.listen_state(self.heater, "binary_sensor.dev29_gpio_5")
        self.listen_state(self.heating, "sensor.dev29_temperature_der")

    def on(self, entity):
        self.log("Toggeling Flow state for 10 sec")
        self.turn_on("switch.dev29_gpio_4")
        self.run_in(self.off,10)

    def off(self, entity=""):
        self.turn_off("switch.dev29_gpio_4")
        self.log("all done")

    def fuse(self, entity="", attribute="", old="", new="", kwargs=""):
#        self.log("new boiler temperature "+str(new))
        if(float(new)<15):
            self.call_service("notify/pb", title="Heating", message="Heater at "+str(new)+"c")
            self.call_service("notify/pb_c", title="Heating", message="Heater at "+str(new)+"c")

    def heater(self, entity="", attribute="", old="", new="", kwargs=""):
#        self.log("new boiler temperature "+str(new))
        if(new=="on"):
            m="Ahhh fuck! Heater in Error mode"
            self.call_service("notify/pb", title="Heating", message=m)
            self.call_service("notify/pb_c", title="Heating", message=m)
        elif(old=="on" and new=="off"):
            m="Puh! Heater not longer in Error mode"
            self.call_service("notify/pb", title="Heating", message=m)
            self.call_service("notify/pb_c", title="Heating", message=m)

    def heating(self, entity="", attribute="", old="", new="", kwargs=""):
       now = datetime.datetime.now()
       yesterday = now - datetime.timedelta(days=1)
       six_h_t = now - datetime.timedelta(hours=6)
       twelf_h_t = now - datetime.timedelta(hours=12)
       six_h = 0
       twelf_h = 0
       #self.log(str(old)+" old vs new "+str(new))
       if(new!=None and old!=None and new!="None" and old!="None"):
          if(float(new)>1 and float(old)<=1):
             self.hd.append(datetime.datetime.now())
          for i in self.hd:
             if(i<yesterday):
                self.hd.remove(i)
             elif(i>six_h_t):
                six_h += 1
                twelf_h += 1
             elif(i>twelf_h_t):
                twelf_h += 1
       #self.log(str(len(self.hd))+" heating cycles in the last 24h, "+str(twelf_h)+" within 12h, "+str(six_h)+" within 6h")
