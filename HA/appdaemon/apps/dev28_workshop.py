import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args:
#

class workshop2World(hass.Hass):

    def initialize(self):
        self.log("Starting workshop2 Service")
        self.ventilation_time = 30
        self.listen_state(self.vent,"switch.dev28_gpio_4")
        self.listen_state(self.countdown,"switch.dev28_countdown")
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
           self.handle = self.run_in(self.air,seconds=20)

    def air(self,kwarg=""):
        self.log("Turn on vent")
        self.turn_on("switch.dev28_gpio_15") # vent

    def countdown(self, entity="", attribute="", old="", new="",kwargs=""):
        if(new=="on"):
           self.log("Turn on Countdown")
           self.turn_on("switch.dev28_gpio_4")
           self.set_state("sensor.dev28_countdown_state",state=self.ventilation_time)
           self.handle_countdown = self.run_in(self.countdown_dec,seconds=60)
        else:
           self.cancel_timer(self.handle_countdown)
           self.set_state("sensor.dev28_countdown_state",state="0")
           self.turn_off("switch.dev28_gpio_4")


    def countdown_dec(self, entity="", attribute="", old="", new="",kwargs=""):
        t = 0
        try:
           t= int(self.get_state("sensor.dev28_countdown_state"))
        except:
           self.log("state error")
        if(t>1):
           self.set_state("sensor.dev28_countdown_state",state=str(t-1))
           self.handle_countdown = self.run_in(self.countdown_dec,seconds=60)
        else:
           self.set_state("switch.dev28_countdown",state="off")
           self.turn_off("switch.dev28_gpio_4")

