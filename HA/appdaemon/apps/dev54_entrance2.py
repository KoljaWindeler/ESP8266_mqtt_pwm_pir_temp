import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class EntranceWorld2(hass.Hass):

    def initialize(self):
        self.log("Starting Entrance Service 2")
        self.listen_state(self.inside_off, "binary_sensor.dev54_motion_1", new = "off", duration = 120)
        self.listen_state(self.inside_on, "binary_sensor.dev54_motion_1", new = "on")
        self.listen_state(self.ring, "binary_sensor.dev54_button", new = "on") #klingel
        self.inside_off()

        self.run_daily(self.six, time(6, 0, 0))
        self.run_daily(self.twentytwo, time(22, 0, 0))
        self.run_daily(self.twentythree, time(23, 0, 0))
        self.run_at_sunrise(self.sunrise, offset = 30 * 60)
        self.run_at_sunset(self.sunset, offset = 15 * 60)
        self.listen_state(self.caro_home, "device_tracker.illuminum_caro", new = "home", duration = 10*60, arg1="Caro home")  # everyone is home for 10 min
        self.listen_state(self.kolja_home, "device_tracker.illuminum_kolja", new = "home", duration = 10*60, arg1="Kolja home")  # everyone is home for 10 min
        self.listen_state(self.approaching, "proximity.caro_home")
        self.listen_state(self.approaching, "proximity.kolja_home")

    ######################################################

    def six(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside_on("6am")
    def twentytwo(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("22pm")
    def twentythree(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("23pm")
    def sunrise(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("Sunrise")
    def sunset(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("Senset")
    def kolja_home(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("Kolja home")
    def caro_home(self, entity="", attribute="", old="", new="", kwargs=""):
        self.outside("Caro home")

    ######################################################

    def ring(self, entity, attribute, old, new,kwargs):
        self.log("========= ring ========================")
        self.outside_wish("on",kwargs)
        self.run_in(self.outside_wish,10*60,w="auto") # turn off after 10 min


    def approaching(self, entity, attribute, old, new,kwargs):
        #self.log("proxy")
        #self.log(dir)
        #self.log(dist)
        if(self.get_state("device_tracker.illuminum_caro") == "home" and self.get_state("device_tracker.illuminum_kolja") == "home"):
            self.log("ignoring approach, both home")
            return 0
        if(attribute == "state"):
            dir = self.get_state(entity, attribute="dir_of_travel")
            dist = self.get_state(entity)
            if(dir == "towards" and attribute == "state"):
                if(int(dist) < 3):
                    self.log("========= approach ========================")
                    #self.log(repr(kwargs["arg1"])
                    self.log(entity+" is approaching home")
                    self.outside_wish("on",kwargs)

    def outside_on(self, title):
        self.log("============= outside on ====================")
        self.log(repr(title))
        self.outside_wish("on")

    def outside(self, title):
        self.log("============== outside ===================")
        self.log(repr(title))
        self.outside_wish("auto")

    def outside_wish(self,w,kwargs=""):
        #self.log(repr(kwargs))

        now = datetime.now().time()
        ele = float(self.get_state("sun.sun", attribute="elevation"))
        self.log("current elevation "+str(ele))
        if(self.sun_up()):
            self.log("sun is up")
        else:
            self.log("sun is down")


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

