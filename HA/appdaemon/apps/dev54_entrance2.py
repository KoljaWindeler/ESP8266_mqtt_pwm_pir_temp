import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class EntranceWorld2(hass.Hass):

    def initialize(self):
        self.log("Starting Entrance Service 2")
        self.listen_state(self.inside_off, "binary_sensor.dev10_motion", new = "off", duration = 120)
        self.listen_state(self.inside_on, "binary_sensor.dev10_motion", new = "on")
        self.listen_state(self.inside_off, "binary_sensor.dev54_motion_1", new = "off", duration = 120)
        self.listen_state(self.inside_on, "binary_sensor.dev54_motion_1", new = "on")
        self.inside_off()

        self.run_daily(self.outside_on, time(6, 0, 0), arg1="6am call")
        self.run_daily(self.outside, time(22, 0, 0),arg1="22pm call")
        self.run_daily(self.outside, time(23, 0, 0), arg2="23pm call")
        self.run_at_sunrise(self.outside, offset = 30 * 60, arg1="Sunrise")
        self.run_at_sunset(self.outside, offset = 15 * 60, arg1="Sunset")
        self.listen_state(self.outside, "device_tracker.illuminum_caro", new = "home", duration = 10*60, arg1="Caro home")  # everyone is home for 10 min
        self.listen_state(self.outside, "device_tracker.illuminum_kolja", new = "home", duration = 10*60, arg1="Kolja home")  # everyone is home for 10 min
        self.listen_state(self.approaching, "proximity.caro_home")
        self.listen_state(self.approaching, "proximity.kolja_home")

    ######################################################

    def approaching(self, entity, attribute, old, new,kwargs):
        #self.log("proxy")
        #self.log(dir)
        #self.log(dist)
        if(attribute == "state"):
            dir = self.get_state(entity, attribute="dir_of_travel")
            dist = self.get_state(entity)
            if(dir == "towards" and attribute == "state"):
                if(int(dist) < 3):
                    self.log("=================================")
                    self.log(entity+" is approaching home")
                    self.outside_wish("on",kwargs)

    def outside_on(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("=================================")
        self.outside_wish("on",kwargs)

    def outside(self, entity="", attribute="", old="", new="", kwargs=""):
        self.log("=================================")
        self.log(entity+" triggered auto")
        self.outside_wish("auto",kwargs)

    def outside_wish(self,w,kwargs=""):
        if("arg1" in kwargs.keys()):
            self.log(kwargs["arg1"])
        now = datetime.now().time()
        ele = float(self.get_state("sun.sun", attribute="elevation"))
        self.log("current elevation "+str(ele))

        if(w=="on"):
            self.log("request to turn on lights")
            if(ele < 2):
                self.log("request granted, sun is low, turning on")
                self.turn_on("light.joiner_outdoor")
                self.log("=================================")
            else:
                self.log("request rejected, sun is up, turning off")
                self.turn_off("light.joiner_outdoor")
                self.log("=================================")
        elif(w=="auto"):
            self.log("request in auto mode")
            if(ele < 2 and now >= time(13,00,00)):
                self.log("sun is low and its evening")
                if (now >= time(23,00,00)):
                    self.log("after 11 turn off")
                    self.turn_off("light.joiner_outdoor")
                    self.log("=================================")
                elif(now >= time(22,00,00) and self.get_state("binary_sensor.everyone_is_home") == "on"):
                    self.log("after 10 and everyone is home, turn off")
                    self.turn_off("light.joiner_outdoor")
                    self.log("=================================")
                else:
                    self.log("before 10 or not everyone is home yet, turn on")
                    self.turn_on("light.joiner_outdoor")
                    self.log("=================================")
            else:
                self.log("its early or the sun is up, turning off")
                self.turn_off("light.joiner_outdoor")
                self.log("=================================")


    ######################################################

    def inside_on(self, entity, attribute, old, new,kwargs):
        ele = float(self.get_state("sun.sun", attribute="elevation"))
        now = datetime.now().time()
        #if (now < time(5,00,00) or now > time(22,30,00)):
        if(ele < 15):
            #self.log("Turn on Chandelair")
            # before 5 or after 22:30, use chandelair
            #if(self.get_state("light.dev54")=="on"):
            #    self.turn_off("light.dev54")
            if(self.get_state("light.dev54_2")=="off"):
                self.turn_on("light.dev54_2")
        #else:
        #    self.log("Turn on LEDs")
        #    # after 5 and before 22:30, use LED
        #    if(self.get_state("light.dev54_2")=="on"):
        #        self.turn_off("light.dev54_2")
        #    if(self.get_state("light.dev54")=="off"):
        #        self.turn_on("light.dev54")

    def inside_off(self, entity="", attribute="", old="", new="",kwargs=""):
        #self.log("Turn all off")

        if(self.get_state("light.dev54_2")=="on"):
            self.turn_off("light.dev54_2")
        #if(self.get_state("light.dev54")=="on"):
        #    self.turn_off("light.dev54")

