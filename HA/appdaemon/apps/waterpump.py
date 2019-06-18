import appdaemon.plugins.hass.hassapi as hass
import datetime, time

#
# Hellow World App
#
# Args: GartenWasserPumpe
#

class PumpWorld(hass.Hass):
    def initialize(self):
        self.log("Starting Pump Service")
        self.turn_off("light.dev17_autostart")
        self.turn_off("light.dev17")
        self.listen_state(self.start,"light.dev17_autostart")

    def start(self, entity, attribute, old, new,kwargs):
        if(self.get_state("light.dev17_autostart")!="on"):
            self.turn_off("light.dev17")
            return

        self.log("################## start ##############################")
        m="Starting pump call"
        self.log(m)
        old_state = 10 # old_state//10 must be !=0
        for i in range(0,3):
            state = 0
            for ii in range(0,41):
                if(self.get_state("light.dev17_autostart")=="on"):
                    if(state//10 != old_state//10):
                        if(state//10==0):
                            m="Ramping up pump 0/40 ("+str(i+1)+"/3)"
                            self.turn_on("light.dev17")
                        elif(state//10==1):
                            m="Wait 10/40 ("+str(i+1)+"/3)"
                        elif(state//10==2):
                            m="Wait 20/40 ("+str(i+1)+"/3)"
                        elif(state//10==3):
                            m="Wait 30/40 ("+str(i+1)+"/3), collect water"
                            self.turn_off("light.dev17")
                        self.set_state("sensor.dev30_state",state=m)
                        self.log("---"+m)
                        old_state = state
                        if(state//10==4):
                            m="Wait 40/40 ("+str(i+1)+"/3), pump on"
                            self.turn_on("light.dev17")
                            state = 0
                            self.set_state("sensor.dev30_state",state=m)
                            self.log("--"+m)
                else:
                    self.turn_off("light.dev17")
                    m="turn off pump, autostart switched off"
                    self.set_state("sensor.dev30_state",state=m)
                    self.log("-"+m)
                    i=99
                    ii=99
                    break
                time.sleep(1)
                state = state + 1
            if(i==99):
                break

        self.log("################## end ##############################")


