import appdaemon.plugins.hass.hassapi as hass
import datetime, time


class workshop2World(hass.Hass):

    def initialize(self):
        self.log("Starting workshop2 Service")
        self.ventilation_time_s = 30
        self.ventilation_time_l = 8*60-1 # 8 std
        self.ventilation_time_l_duty = 0.1
        self.listen_state(self.vent,"switch.dev28_gpio_4")
        self.listen_state(self.countdown,"switch.dev28_countdown")
        self.listen_state(self.countdown,"switch.dev28_countdown_l")
        self.set_state("sensor.dev28_countdown_state",state="0")

    def vent(self, entity="", attribute="", old="", new="",kwargs=""):
        self.log("Toggle gpio 4")
        if(new=="off"):
           self.log("stopping circulation")
           self.turn_off("switch.dev28_gpio_15") # vent
           self.turn_off("switch.dev28_gpio_16")
           self.turn_off("switch.dev28_gpio_4")
           try:
              self.cancel_timer(self.handle)
           except:
              pass
        else:
           self.log("starting circulation")
           self.turn_on("switch.dev28_gpio_16")
           self.turn_on("switch.dev28_gpio_4")
           self.handle = self.run_in(self.air,delay=20)

    def air(self,kwarg=""):
        self.log("Turn on vent")
        self.turn_on("switch.dev28_gpio_15") # vent

    def countdown(self, entity="", attribute="", old="", new="",kwargs=""):
        if(new=="on"):
           self.log("Turn on Countdown")
           self.turn_on("switch.dev28_gpio_4")
           d = self.ventilation_time_s
           if(entity=="switch.dev28_countdown_l"):
              self.log("long prog")
              d = self.ventilation_time_l
           self.set_state("sensor.dev28_countdown_state",state=d)
           self.handle_countdown = self.run_in(self.countdown_dec,delay=60)
        else:
           try:
               self.cancel_timer(self.handle_countdown)
           except:
               pass
           self.set_state("sensor.dev28_countdown_state",state="0")
           self.turn_off("switch.dev28_gpio_4")


    def countdown_dec(self, entity="", attribute="", old="", new="",kwargs=""):
        t = 0
        s = "off"
        try:
           t = int(self.get_state("sensor.dev28_countdown_state"))
           s = self.get_state("switch.dev28_gpio_4")
        except:
           self.log("state error")
        if(t>self.ventilation_time_s):
           a = t % 60
           b = (1-self.ventilation_time_l_duty) * 60
           if(a>=b and s=="off"): # should be on
              self.log("b>a "+str(b)+">"+str(a)+", s="+str(s)+" t="+str(t))
              self.turn_on("switch.dev28_gpio_4")
           elif(a<b and s=="on"): # should be off
              self.log("b<a "+str(b)+">"+str(a)+", s="+str(s)+" t="+str(t))
              self.turn_off("switch.dev28_gpio_4")
           else:
              self.log(str(b)+">"+str(a)+", s="+str(s)+" t="+str(t))


        if(t>1):
           self.set_state("sensor.dev28_countdown_state",state=str(t-1))
           self.handle_countdown = self.run_in(self.countdown_dec,delay=60)
        else:
           self.set_state("switch.dev28_countdown",state="off")
           self.set_state("switch.dev28_countdown_l",state="off")
           self.turn_off("switch.dev28_gpio_4")

