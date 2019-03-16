import appdaemon.plugins.hass.hassapi as hass
import datetime, time

#
# Hellow World App
#
# Args:
#

class SprinklerWorld(hass.Hass):

    def g(self,entity,default):
        try:
            r = self.get_state(entity)
            if(not(isinstance(r,str))):
                self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
                r = default
            if(r=="unknown"):
                self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
                r = default
        except:
            r = default
            self.call_service("notify/pb", title="Irrigation", message="Problem getting "+entity+" using default value "+default)
        return r

    def check_stop(self):
        if(self.g("light.dev17","off")=="off" or self.g("input_boolean.irrigation","off")=="off"): 
            return True
        else:
            return False

    def initialize(self):
        self.log("Starting Sprinkler Service")
        self.listen_state(self.start,"input_boolean.irrigation", new = "on")
        self.turn_off("light.dev17")


    def start(self, entity, attribute, old, new,kwargs):
        #self.call_service("notify/pb", title="Irrigation", message="Starting")
        self.log("################################################")
        self.log("Starting Sprinker call")
        self.set_state("sensor.dev30_state",state="Starting")
        max_time = 15 #30

        config = [
            [['light.dev30_4'],1], # one valve, 2xt200, regular time
            [['light.dev30_2'],1], # one valve, 3xt200, regular time
            [['light.dev30_1','light.dev30_3'],1.5] # two valves, 150% time
                ]

        #today_max_temp = int(float(self.g("sensor.yweather_temperature_max","28")))
        today_max_temp = int(float(self.g("sensor.dark_sky_daytime_high_temperature_0","28")))
        if(today_max_temp > 28):
            max_time = 30
        elif(today_max_temp < 22):
            max_time = 0

        self.log("Today max temp is "+str(today_max_temp)+", will irrigate for "+str(max_time)+ " min")
        rain_today = int(float(self.g("sensor.dev30_uptime","0"))/5)
        max_time = max(max_time - rain_today,0)
        self.log("There was already "+str(rain_today)+" min of rain, so I'll irrigate for "+str(max_time)+" now")

        if(max_time>0):
            self.log("################################################")
            self.log("Preparing Pump")
            self.set_state("sensor.dev30_state",state="Preparing pump")
            self.log("Activating valve power")
            if(int(self.g("sensor.dev30_update","0"))>0):
                #self.call_service("notify/pb", title="Irrigation", message="Valves offline, waiting up to 5 min")
                self.set_state("sensor.dev30_state",state="Valve offline, wait 0/50")
                for wait in range(0,50):
                    time.sleep(6)
                    self.set_state("sensor.dev30_state",state="Valve offline, wait "+str(wait)+"/50")
                    if(int(self.g("sensor.dev30_update","0"))==0):
                        break
            if(int(self.g("sensor.dev30_update","0"))>0):
                #self.call_service("notify/pb", title="Irrigation", message="Valves still offline, giving up")
                self.set_state("sensor.dev30_state",state="Valve offline, give up")
                exit()
            else:
                #self.call_service("notify/pb", title="Irrigation", message="Valves online, here we go")
                self.set_state("sensor.dev30_state",state="Valve online")


            self.turn_on("light.dev30_pow")
            self.log("Opening all valves to reduce pump pressure")
            self.set_state("sensor.dev30_state",state="Open all valves")
            for ring in range(0,len(config)):
                for sprinkler in range(0,len(config[ring][0])):
                    self.log("Valve "+config[ring][0][sprinkler]+" open")
                    self.turn_on(config[ring][0][sprinkler])

            for i in range(0,3):
                self.set_state("sensor.dev30_state",state="Ramp up "+str(i+1)+"/3, 00/40")
                self.log("Ramping up pump "+str(i+1)+" / 3")
                self.turn_on("light.dev17")
                time.sleep(10)
                self.set_state("sensor.dev30_state",state="Ramp up "+str(i+1)+"/3, 10/40")
                self.log("Wait 10/40")
                time.sleep(10)
                self.set_state("sensor.dev30_state",state="Ramp up "+str(i+1)+"/3, 20/40")
                self.log("Wait 20/40")
                time.sleep(10)
                self.log("Wait 30/40, turn off to collect water")
                self.set_state("sensor.dev30_state",state="Ramp up "+str(i+1)+"/3, 30/40")
                self.turn_off("light.dev17")
                time.sleep(10)
                self.turn_on("light.dev17")
                self.log("Wait 40/40, pump turned on again")
                self.set_state("sensor.dev30_state",state="Ramp up "+str(i+1)+"/3, 40/40")
                time.sleep(1)
                if(self.check_stop()):
                    break



            self.turn_on("light.dev17")
            self.log("Pump ready")
            time.sleep(2) # give a little time to g states
            self.log("Closing all valves")
            for ring in range(0,len(config)):
                for sprinkler in range(0,len(config[ring][0])):
                    self.turn_off(config[ring][0][sprinkler])
                    self.log("Valve "+config[ring][0][sprinkler]+" closed")
            self.log("Preperation completed, starting the irrigation")
            self.log("################################################")
            self.set_state("sensor.dev30_state",state="Preparation done")



            for ring in range(0,len(config)):
                if(not(self.check_stop())):
                    self.log("Turn ring "+str(ring+1)+" on.")
                    D = max_time * config[ring][1]
                    for sprinkler in range(0,len(config[ring][0])):
                        self.turn_on(config[ring][0][sprinkler])
                    while D > 0:
                        self.log("Currently running ring "+str(ring+1)+" for further "+str(D)+" min")
                        self.set_state("sensor.dev30_state",state="Ring "+str(ring+1)+" -> "+str(D)+" min")
                        D = D-1
                        if(self.check_stop()):
                            self.call_service("notify/pb", title="Irrigation", message="Pump off, stopping")
                            self.log("Pump off, stopping irrigation")
                            self.set_state("sensor.dev30_state",state="Stopping")
                            break
                        time.sleep(60)
                    self.log("Turn ring "+str(ring+1)+" off.")
                    for sprinkler in range(0,len(config[ring][0])):
                        self.turn_off(config[ring][0][sprinkler])
                else:
                    self.log("Skipping ring "+str(ring+1)+" pump is off")

            self.set_state("sensor.dev30_state",state="Shutting down")
            self.log("Shutting down valve power")
            self.turn_off("light.dev30_pow")
            self.log("Shutting down pump")
            self.turn_off("light.dev17")
            #self.call_service("notify/pb", title="Irrigation", message="All done")

        #else:
            #self.call_service("notify/pb", title="Irrigation", message="Skipping, no rain needed")

        self.turn_on("light.dev30_reset_rain_time")
        time.sleep(1)
        self.turn_off("light.dev30_reset_rain_time")
        self.log("All done, enjoy your evening")
        self.turn_off("input_boolean.irrigation")
        self.log("All done, enjoy your evening")
        self.set_state("sensor.dev30_state",state="Done")
        self.log("################################################")
