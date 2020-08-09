import appdaemon.plugins.hass.hassapi as hass
from datetime import datetime, time
import json
#
# Hellow World App
#
# Args: GartenWasserWeathere
#

class OwntracksWorld(hass.Hass):
	def initialize(self):
		self.log("Starting owntracks Service")
		self.kolja = json.dumps(self.get_state("device_tracker.illuminum_kolja",attribute="all"))
		self.caro = json.dumps(self.get_state("device_tracker.illuminum_caro",attribute="all"))
		self.run_in(self.check,2*60*60)

	def check(self, entity='', attribute='', old='', new='',kwargs=''):
		test_kolja = json.dumps(self.get_state("device_tracker.illuminum_kolja",attribute="all"))
		test_caro = json.dumps(self.get_state("device_tracker.illuminum_caro",attribute="all"))
		t="owntracks"
		m="is your owntracks off?"
		now = datetime.now().time()

		if((self.kolja == test_kolja) and (now >= time(9,0,0))):
			self.call_service("notify/pb", title=t, message=m)
			self.log("notify kolja")
		if((self.caro == test_caro) and (now >= time(9,0,0))):
			self.call_service("notify/pb_c", title=t, message=m)
			self.log("notify caro")
		self.kolja = test_kolja
		self.caro = test_caro
		self.run_in(self.check,2*60*60)
