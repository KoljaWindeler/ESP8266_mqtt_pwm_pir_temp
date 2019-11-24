import appdaemon.plugins.hass.hassapi as hass
import datetime, time,random

#
# Hellow World App
#
# Args: GartenWasserWeathere
#

class gmusicWorld(hass.Hass):
    def initialize(self):
        self.log("Starting gmusic Service")
        self.listen_state(self.play,"binary_sensor.dev3_button1s", new = "on")

    def play(self, entity="", attribute="", old="", new="", kwargs=""):
        self.turn_on("light.joiner_kitchen");
        self.turn_on("light.joiner_livingroom");
        self.turn_on("script.start_chromaudio")
        self.handle = self.run_in(self.set_vol, seconds=3) 
        self.turn_off("media_player.gmusic_player")
        self.set_state("input_select.gmusic_player_source",state="Station")
        self.set_state("input_select.gmusic_player_station",state="I'm Feeling Lucky")

    def set_vol(self, entity="", attribute="", old="", new="", kwargs=""):
        self.call_service("media_player/volume_set", entity_id="media_player.yamaha_receiver", volume_level="0.4")
        self.turn_on("media_player.gmusic_player")

