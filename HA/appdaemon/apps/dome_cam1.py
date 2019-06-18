import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import subprocess


class DomeCamWorld(hass.Hass):

    def initialize(self):
        self.log("Starting DomeCam Service")
        self.listen_state(self.Snapshot, "binary_sensor.dev54_button", new = "on")
        self.Snapshot()

    def Snapshot(self, entity="", attribute="", old="", new="",kwargs=""):
        self.log("DomeCam Snapshot")
        subprocess.call(["ffmpeg", "-loglevel", "fatal", "-i", "rtsp://192.168.2.10//user=admin_password=_channel=1_stream=0.sdp", "-vframes", "1", "-r", "1", "-y", "/tmp/dome1_1.jpg"])
        self.call_service("notify/pb", title="Ding Dong", message="Foto", data={"file": "/tmp/dome1_1.jpg"})
