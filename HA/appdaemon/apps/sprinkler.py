import appdaemon.plugins.hass.hassapi as hass
import datetime, time

#
# Hellow World App
#
# Args:
#

class SprinklerWorld(hass.Hass):

    def initialize(self):
        self.log("Starting Sprinkler Service")
        self.listen_state(self.start,"input_boolean.irrigation", new = "on")


    def start(self, entity, attribute, old, new,kwargs):
        #self.set_state("sensor.dev30_ssid",state = "42")
        #exit()

        self.call_service("notify/pb", title="Irrigation", message="Starting")
        self.log("################################################")
        self.log("Starting Sprinker call")
        max_time = 20 #30

        config = [
            [['light.dev30_4'],1], # one valve, 2xt200, regular time
            [['light.dev30_2'],1], # one valve, 3xt200, regular time
            [['light.dev30_1','light.dev30_3'],1.5] # two valves, 150% time
                ]

        today_max_temp = int(float(self.get_state("sensor.dark_sky_daily_high_temperature")))
        if(today_max_temp > 28):
            max_time = 30
        elif(today_max_temp < 15):
            max_time = 0

        self.log("Today max temp is "+str(today_max_temp)+", will irrigate for "+str(max_time)+ " min")
        if(self.get_state("sensor.dev30_uptime")!="unknown"):
            rain_today = int(float(self.get_state("sensor.dev30_uptime")))
        else:
            rain_today = 0
        max_time = max(max_time - rain_today,0)
        self.log("There was already "+str(rain_today)+" min of rain, so I'll irrigate for "+str(max_time)+" now")

        if(max_time>0):
            self.log("################################################")
            self.log("Preparing Pump")
            self.log("Activating valve power")
            if(int(self.get_state("sensor.dev30_update"))>0):
                self.call_service("notify/pb", title="Irrigation", message="Valves offline, waiting up to 5 min")
                for wait in range(0,50):
                    time.sleep(6)
                    if(int(self.get_state("sensor.dev30_update"))==0):
                        break
            if(int(self.get_state("sensor.dev30_update"))>0):
                self.call_service("notify/pb", title="Irrigation", message="Valves still offline, giving up")
                exit()
            else:
                self.call_service("notify/pb", title="Irrigation", message="Valves online, here we go")

            self.turn_on("light.dev30_pow")
            self.log("Opening all valves to reduce pump pressure")
            for ring in range(0,len(config)):
                for sprinkler in range(0,len(config[ring][0])):
                    self.log("Valve "+config[ring][0][sprinkler]+" open")
                    self.turn_on(config[ring][0][sprinkler])

            self.log("Ramping up pump")
            self.turn_on("light.dev17")
            #self.call_service("mqtt/publish", topic="dev17/s/light", payload="ONFOR60") # max 1 min on
            time.sleep(10)
            self.log("Wait 10/40")
            time.sleep(10)
            self.log("Wait 20/40")
            time.sleep(10)
            self.log("Wait 30/40, turn off to collect water")
            self.turn_off("light.dev17")
            time.sleep(10)
            #tt = 0
            #for i in range(0,len(config)):
            #    tt = tt + config[i][1]
            #tt=tt*max_time*60
            #self.call_service("mqtt/publish", topic="dev17/s/light", payload="ONFOR"+str(tt))
            self.turn_on("light.dev17")
            self.log("Wait 40/40, pump turned on again")
            self.log("Pump ready")
            time.sleep(2) # give a little time to get states
            self.log("Closing all valves")
            for ring in range(0,len(config)):
                for sprinkler in range(0,len(config[ring][0])):
                    self.turn_off(config[ring][0][sprinkler])
                    self.log("Valve "+config[ring][0][sprinkler]+" closed")
            self.log("Preperation completed, starting the irrigation")
            self.log("################################################")


            for ring in range(0,len(config)):
                if(self.get_state("light.dev17")!="off"):
                    self.log("Turn ring "+str(ring+1)+" on.")
                    D = max_time * config[ring][1]
                    for sprinkler in range(0,len(config[ring][0])):
                        self.turn_on(config[ring][0][sprinkler])
                    while D > 0:
                        self.log("Currently running ring "+str(ring+1)+" for further "+str(D)+" min")
                        D = D-1
                        if(self.get_state("light.dev17")=="off" or self.get_state("input_boolean.irrigation")=="off"): 
                            self.call_service("notify/pb", title="Irrigation", message="Pump off, stopping")
                            self.log("Pump off, stopping irrigation")
                            break
                        time.sleep(60)
                    self.log("Turn ring "+str(ring+1)+" off.")
                    for sprinkler in range(0,len(config[ring][0])):
                        self.turn_off(config[ring][0][sprinkler])
                else:
                    self.log("Skipping ring "+str(ring+1)+" pump is off")

            self.log("Shutting down valve power")
            self.turn_off("light.dev30_pow")
            self.log("Shutting down pump")
            self.turn_off("light.dev17")
            self.call_service("notify/pb", title="Irrigation", message="All done")

        else:
            self.call_service("notify/pb", title="Irrigation", message="Skipping, no rain needed")

        self.turn_off("input_boolean.irrigation")
        self.log("All done")
        self.log("################################################")
