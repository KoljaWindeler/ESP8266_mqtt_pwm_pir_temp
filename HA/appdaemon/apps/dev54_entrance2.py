import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import wait

class EntranceWorld2(hass.Hass):

    def initialize(self):
        self.log("Starting Entrance Service 2")
        wait.wait_available(self,["binary_sensor.dev54_button","binary_sensor.dev17_motion","device_tracker.illuminum_caro","device_tracker.illuminum_kolja","proximity.caro_home","proximity.kolja_home"],False)

        self.ring_cnt = [0,0]
        self.listen_state(self.ring, "binary_sensor.dev54_button", new = "on") #klingel
        self.listen_state(self.backdoor, "binary_sensor.dev17_motion", new = "on") #klingel

        self.run_daily(self.six, time(6, 0, 0))
        self.run_daily(self.twentytwo, time(22, 0, 0))
        self.run_daily(self.twentythree, time(23, 0, 0))
        try:
           self.run_at_sunrise(self.sunrise, offset = 30 * 60)
           self.run_at_sunset(self.sunset, offset = 15 * 60)
        except:
           pass
        self.listen_state(self.caro_home, "device_tracker.illuminum_caro", new = "home", duration = 10*60, arg1="Caro home")  # everyone is home for 10 min
        self.listen_state(self.kolja_home, "device_tracker.illuminum_kolja", new = "home", duration = 10*60, arg1="Kolja home")  # everyone is home for 10 min
        self.listen_state(self.approaching, "proximity.caro_home")
        self.listen_state(self.approaching, "proximity.kolja_home")
        #self.backdoor()


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
        self.log("========= ring ========================") # log output
        self.outside_wish("on!",kwargs) # turn light on
        fn = "/tmp/dome1_"+str(self.ring_cnt[0]) # save filename without filetype ending
        self.ring_cnt[0] = (self.ring_cnt[0] + 1) % 10 # inc
        self.call_service("camera/record",   entity_id="camera.cam_dome1", filename=fn+".mp4", duration="20", lookback="5") # save video
        self.call_service("camera/snapshot", entity_id="camera.cam_dome1", filename=fn+".jpg") # save snapshot
        self.notify_vid(arg={"arg":{"t":"Frontdoor","m":"Ding Dong","d":{"file":""}}}) # send info
        self.run_in(self.notify_vid,1, arg={"t":"Frontdoor image","m":"Sensor triggered","d":{"file": fn+".jpg"}}) # send image
        self.run_in(self.notify_vid,20,arg={"t":"Frontdoor video","m":"http://192.168.2.84:8081/"+fn.replace("/tmp/","")+".mp4","d":{"file":""}}) # send link to video
        self.run_in(self.outside,5*60) # turn off after 5 min

    def backdoor(self, entity="", attribute="", old="", new="",kwargs=""):
        self.log("========= backdoor ========================") # log output
        fn = "/tmp/dome2_"+str(self.ring_cnt[1]) # save filename without firetype ending
        self.ring_cnt[1] = (self.ring_cnt[1] + 1) % 10 # inc
        self.call_service("camera/record",   entity_id="camera.cam_dome2", filename=fn+".mp4", duration="20", lookback="5") # save video
        self.call_service("camera/snapshot", entity_id="camera.cam_dome2", filename=fn+".jpg") # save snapshot
        if(self.get_state("input_boolean.alarm_system") == "on" and self.get_state("binary_sensor.someone_is_home") == "off"):
           self.notify_vid(arg={"arg":{"t":"Backdoor","m":"Triggered","d":{"file":""}}}) # send info
           self.run_in(self.notify_vid,1, arg={"t":"Backdoor image","m":"Sensor triggered","d":{"file": fn+".jpg"}}) # send image
           self.run_in(self.notify_vid,20,arg={"t":"Backdoor video","m":"http://192.168.2.84:8081/"+fn.replace("/tmp/","")+".mp4","d":{"file":""}}) # send link to video

    def notify_vid(self, arg):
        #self.log(arg)
        t=arg["arg"]["t"]
        m=arg["arg"]["m"]
        d=arg["arg"]["d"]
        self.log("sending notification: "+t)
        if(d["file"]!=""):
           self.call_service("notify/pb",   title=t, message=m, data=d)
           self.call_service("notify/pb_c", title=t, message=m, data=d)
        else:
           self.call_service("notify/pb",   title=t, message=m)
           self.call_service("notify/pb_c", title=t, message=m)

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
        rising = self.get_state("sun.sun", attribute="rising")
        self.log("current elevation "+str(ele))

        if(w=="on!"):
            self.log("COMMAND! to turn on lights")
            self.turn_on("light.joiner_outdoor")
            self.log("=================================")
        elif(w=="on"):
            self.log("request to turn on lights")
            if(ele < 2):
                self.log("request granted, sun is low, turning on")
                self.turn_on("light.joiner_outdoor")
                self.log("=================================")
            else:
                self.log("request rejected, sun is up, turning off")
                self.turn_off("light.joiner_outdoor")
                self.log("=================================")
        else: #if(w=="auto"):
            self.log("request in auto mode")
            if(now >= time(23,00,00)):
                self.log("after 11 turn off")
                self.turn_off("light.joiner_outdoor")
            elif(now >= time(22,00,00) and self.get_state("binary_sensor.everyone_is_home") == "on"):
                self.log("after 10 and everyone is home, turn off")
                self.turn_off("light.joiner_outdoor")
            elif(ele < 2 and now >= time(13,0,0)):
                self.log("sun low and falling, must be evening, turn on")
                self.turn_on("light.joiner_outdoor")
            elif(ele >= 2):
                self.log("sun is up, turn off")
                self.turn_off("light.joiner_outdoor")
            elif(now >= time(6,0,0)):
                self.log(">= six AM but still dark, turn on")
                self.turn_on("light.joiner_outdoor")
            else:
                self.log("before six in the nigth, turn off")
                self.turn_off("light.joiner_outdoor")
            self.log("=================================")



