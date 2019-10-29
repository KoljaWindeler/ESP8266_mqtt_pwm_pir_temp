import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

#
# Hellow World App
#
# Args: GartenWasserWeathere
#

class DingDongWorld(hass.Hass):
    def initialize(self):
        self.log("Starting dingdong Service")
        self.listen_state(self.ring, "binary_sensor.dev54_button", new = "on") #klingel

    def ring(self, entity='', attribute='', old='', new='',kwargs=''):
        self.log("### DingDongEvent ###")
        now = datetime.now().time()
        if(now <= time(19,00,00) and now>=time(7,0,0)):
           self.call_service("media_player/play_media",entity_id="media_player.badezimmer", media_content_id="http://192.168.2.84:8123/local/dingdong.mp3", media_content_type="music")
           self.call_service("media_player/volume_set",entity_id="media_player.badezimmer", volume_level="0.8")
