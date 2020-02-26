
import appdaemon.plugins.hass.hassapi as hass
import datetime, time


#
# Hellow World App
#
# Args: 12 vrnt, 13 window
#

class mr_ventWorld(hass.Hass):

    def initialize(self):
        self.log("Starting movieroom_vent Service")
        self.ventilation_time = 10
        self.listen_state(self.countdown,"switch.dev60_countdown")
        self.set_state("sensor.dev60_countdown_state",state="0")

    def vent(self, entity="", attribute="", old="", new="",kwargs=""):
        if(new=="off"):
           self.log("stopping circulation")
           # save light state
           led_state = self.get_state("light.dev10")
           self.turn_off("switch.dev60_gpio_12") # vent
           self.turn_off("switch.dev60_gpio_13") # window
           #restore light state
           if(led_state == "on"):
              self.turn_on("light.dev10")
           else:
              self.turn_off("light.dev10")
           try:
              self.cancel_timer(self.handle)
           except:
              pass
        else:
           self.log("starting circulation")
           self.turn_on("switch.dev60_gpio_13") # window
           self.handle = self.run_in(self.air,delay=10)

    def air(self,kwarg=""):
        self.log("Turn on vent")
        self.turn_on("switch.dev60_gpio_12") # vent

    def countdown(self, entity="", attribute="", old="", new="",kwargs=""):
        if(new=="on"):
           self.log("Turn on Countdown")
           self.vent(new="on")
           self.set_state("sensor.dev60_countdown_state",state=self.ventilation_time)
           self.handle_countdown = self.run_in(self.countdown_dec,delay=60)
        else:
           try:
              self.cancel_timer(self.handle_countdown)
           except:
              pass
           self.set_state("sensor.dev60_countdown_state",state="0")
           self.vent(new="off")


    def countdown_dec(self, entity="", attribute="", old="", new="",kwargs=""):
        t = 0
        try:
           t= int(self.get_state("sensor.dev60_countdown_state"))
        except:
           self.log("state error")
        if(t>1):
           self.set_state("sensor.dev60_countdown_state",state=str(t-1))
           self.handle_countdown = self.run_in(self.countdown_dec,delay=60)
        else:
           self.set_state("switch.dev60_countdown",state="off")
           self.vent(new="off")

