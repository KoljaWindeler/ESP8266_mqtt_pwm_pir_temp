import appdaemon.plugins.hass.hassapi as hass
import requests
from datetime import datetime, time
#
# Hellow World App
#
# Args: GartenWasserWeathere
#

class gmusicWorld(hass.Hass):
    def initialize(self):
        self.log("Starting gmusic Service")
        self.listen_state(self.toggle,"binary_sensor.dev3_button1s", new = "on")

    def toggle(self, entity="", attribute="", old="", new="", kwargs=""):
        now = datetime.now().time()
        if(self.get_state("light.joiner_livingroom")=="on" and self.get_state("light.dev14")=="on"):
           self.log("Light seams to be on, turning off")
           self.turn_all_lights_off()
           self.turn_music_off()
           if(now > time(6,0,0) and now < time(8,0,0)):
              self.log("Between 6 and 8, opening garage")
              self.open_garage()
        else:
           self.log("Light seams to be off, turning on")
           self.turn_all_lights_on()
           self.turn_music_on()

    ###############    ###############    ###############

    def turn_music_on(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_on("script.start_chromaudio")
        self.play()

    def turn_music_off(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_on("script.seq_tv_off")
        self.turn_off("media_player.gmusic_player")

    def open_garage(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_on("switch.dev8")
        requests.get(url="http://192.168.2.110:2323/?cmd=textToSpeech&text=opening+Garage&password=asdf")

    def turn_all_lights_on(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_all_off_next = True
        self.turn_on("light.joiner_kitchen")
        self.turn_on("light.joiner_livingroom")

    def turn_all_lights_off(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_all_off_next = False
        self.turn_off("light.joiner_kitchen")
        self.turn_off("light.joiner_livingroom")

    ###############    ###############    ###############

    def play(self, entity="", attribute="", old="", new="", kwargs=""):
        self.handle = self.run_in(self.set_vol, seconds=3)
        self.turn_off("media_player.gmusic_player")
        self.set_state("input_select.gmusic_player_source",state="Station")
        self.set_state("input_select.gmusic_player_station",state="I'm Feeling Lucky")

    def set_vol(self, entity="", attribute="", old="", new="", kwargs=""):
        self.call_service("media_player/volume_set", entity_id="media_player.yamaha_receiver", volume_level="0.4")
        self.turn_on("media_player.gmusic_player")

