import appdaemon.plugins.hass.hassapi as hass
import datetime, time,random

#
# Hellow World App
#
# Args: GartenWasserWeathere
#

class WeatherWorld(hass.Hass):
    def initialize(self):
        self.log("Starting Weather Service")
        self.set_state("camera.cam_weather1", state = "on2" )
        time = datetime.datetime.now() + datetime.timedelta(minutes=10)
        self.run_every(self.run_every_c, time, 10 * 60) # every 10 min

    def run_every_c(self, entity='', attribute='', old='', new='',kwargs=''):
#        self.log("### trigger reload ###")
        self.set_state("sensor.weather_reload_trigger", state = str(random.randint(99,999)))


