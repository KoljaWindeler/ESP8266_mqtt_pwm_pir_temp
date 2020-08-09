import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time

class SecurityWorld(hass.Hass):

	def initialize(self):
		self.status = 0 # off
		self.log("Starting Security Service")
		self.listen_state(self.light, "switch.alarm_light_on")
		self.listen_state(self.sound, "switch.alarm_sound_on")
		self.listen_state(self.generic, "switch.alarm_on")


	######################################################

	def generic(self, entity="", attribute="", old="", new="", kwargs=""):
		if(new=="on"):
			self.status = 1

			### send MSG to make all aware ###
			t="EMERGENCY"
			m="Mode turned on"
			self.call_service("notify/pb", title=t, message=m)

			### start recording ###
			self.cam_recording_id = 0
			self.cams()

			# start blinking outside lights
			self.light_toggle()

		else:
			self.status = 0

	def sound(self, entity="", attribute="", old="", new="", kwargs=""):
		self.log("Turning Sound "+str(new))
		if(new=="on"):
			self.log("starting profiles")
			self.turn_on("script.start_chromaudio")
			#self.turn_on("script.beamer_start_chrome")
			self.run_in(self.sound_volume,10)

	def sound_volume(self,kwargs=""):
		if(self.get_state("switch.alarm_sound_on")=="on"):
			self.log("setting volumes")
			# all google devices
			# self.call_service("media_player/volume_set", entity_id="media_player.yamaha_receiver", volume_level: "0")
			#self.call_service("media_player/volume_set", entity_id="media_player.kuche", volume_level= "0.4")
			#self.call_service("media_player/volume_set", entity_id="media_player.badezimmer", volume_level= "0.4")

			# basement direct
			self.call_service("media_player/volume_set", entity_id="media_player.yamaha_receiver", volume_level= "0.8")
			# cellar iterativ
			#self.call_service("remote/send_command", entity_id="remote.beamer_harmony", data={"command": "VolumeUp", "device": "64439581", "num_repeats":"50", "delay_secs": "0.0" })
			self.run_in(self.sound_play,1)
			# google home teil?

	def sound_play(self, kwargs=""):
		if(self.get_state("switch.alarm_sound_on")=="on"):
			self.log("play")
			# start playback
			m = "http://192.168.2.84:8123/local/s.mp3"
			#self.call_service("media_player/play_media",entity_id="media_player.chromecast0864", media_content_id=m, media_content_type="music")
			#self.call_service("media_player/play_media",entity_id="media_player.kueche", media_content_id=m, media_content_type="music")
			#self.call_service("media_player/play_media",entity_id="media_player.badezimmer", media_content_id=m, media_content_type="music")
			self.call_service("media_player/play_media",entity_id="media_player.jkw_cast2", media_content_id=m, media_content_type="music")
			self.run_in(self.sound_play,8) # accordning to length of file TODO

	def light(self, entity="", attribute="", old="", new="", kwargs=""):
		self.log("Turning Light "+str(new))

		# turn all lights on/off
		all = self.get_state("light")
		for i in all:
			if(new=="on"):
				self.turn_on(i)
			if(new=="off"):
				self.turn_off(i)

		# start/stop blinking outside lights
		self.light_toggle()

	def light_toggle(self, kwargs=""):
		if(self.status and self.get_state("switch.alarm_light_on")=="on"):
			self.toggle("light.dev54_3")
			self.toggle("light.dev54_4")
			self.run_in(self.light_toggle,1)

	def cams(self):
		if(self.status):
			self.call_service("camera/record", entity_id="camera.cam_dome1", filename="/tmp/alarm_cam1_"+str(self.cam_recording_id)+".mp4", duration="60", lookback="5") # save video
			self.call_service("camera/record", entity_id="camera.cam_dome2", filename="/tmp/alarm_cam2_"+str(self.cam_recording_id)+".mp4", duration="60", lookback="5") # save video
			self.call_service("camera/record", entity_id="camera.cam_dome3", filename="/tmp/alarm_cam3_"+str(self.cam_recording_id)+".mp4", duration="60", lookback="5") # save video
			self.cam_recording_id += 1
			self.run_in(self.cams,60)
