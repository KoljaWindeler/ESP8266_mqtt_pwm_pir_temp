"""
Custom component to grab data from a energy_calc solar inverter.

@ Author	  : Kolja Windeler
@ Date		  : 2020/08/24
@ Description : Grabs and parses the data of a energy_calc inverter

https://github.com/home-assistant/core/blob/dev/homeassistant/components/recorder/models.py
https://github.com/home-assistant/core/blob/dev/homeassistant/components/statistics/sensor.py
"""
import logging
from homeassistant.helpers.entity import Entity, async_generate_entity_id
from homeassistant.components.recorder.models import States
from homeassistant.components.recorder.util import execute, session_scope
from homeassistant.components.sensor import ENTITY_ID_FORMAT
from homeassistant.const import (CONF_NAME, CONF_ENTITY_ID, EVENT_HOMEASSISTANT_START, STATE_UNKNOWN,ATTR_UNIT_OF_MEASUREMENT)
from homeassistant.helpers.event import async_track_state_change
from homeassistant.core import callback


from pytz import timezone
from tzlocal import get_localzone
import datetime
import traceback
from .const import *

_LOGGER = logging.getLogger(__name__)


async def async_setup_platform(hass, config, async_add_entities, discovery_info=None):
	"""Run setup via YAML."""
	_LOGGER.debug("Config via YAML")
	if(config is not None):
		async_add_entities([energy_calc_sensor(hass, config)], True)


async def async_setup_entry(hass, config, async_add_devices):
	"""Run setup via Storage."""
	_LOGGER.debug("Config via Storage/UI")
	if(len(config.data) > 0):
		async_add_devices([energy_calc_sensor(hass, config.data)], True)


class energy_calc_sensor(Entity):
	"""Representation of a Sensor."""

	def __init__(self, hass, config):
		"""Initialize the sensor."""
		self._state_attributes = None
		self._state = None

		self._crashed = False
		self._sample = 0

		self._name = config.get(CONF_NAME)
		self._icon = config.get(CONF_ICON)
		self._net = config.get(CONF_NET)
		self._gen = config.get(CONF_GEN)

		_LOGGER.debug("energy_calc config: ")
		_LOGGER.debug("\tname: " + self._name)
		_LOGGER.debug("\ticon: " + str(self._icon))

		self.energy_calc = {
			'extra': {
				'generator_w': 0,
				'net_w' : 0,
				'home_w' : 0,
				'home_from_solar_per' : 0,
				'generated_kwh': 0,
				'feed_in_kwh': 0,
				'feed_out_kwh': 0,
				'self_consumed_kwh': 0,
				'total_consumed_kwh': 0,
				'last_updated_gen': None,
				'last_updated_net': None
			},
			'self_consumed_per': 0,
		}

		self.entity_id = async_generate_entity_id(ENTITY_ID_FORMAT, "energy_calc", hass=hass)

	async def async_added_to_hass(self):
		@callback
		def async_listener(entity, old_state, new_state):
			"""Handle the sensor state changes."""
			#_LOGGER.error("callback")
			self.add_state(entity,old_state,new_state)

		@callback
		def async_energy_calc_startup(event):
			"""Add listener and get recorded state."""
			_LOGGER.debug("Startup for %s", self._gen)
			async_track_state_change(self.hass, self._gen, async_listener)
			async_track_state_change(self.hass, self._net, async_listener)
			if "recorder" in self.hass.config.components:
				# Only use the database if it's configured
				#_LOGGER.error("+++++++++++++  recorder in config +++++++++++++")
				self.hass.async_create_task(self._async_initialize_from_database())
			#else:
			#	_LOGGER.error("+++++++++++++ no recorder in config +++++++++++++")

		self.hass.bus.async_listen_once(EVENT_HOMEASSISTANT_START, async_energy_calc_startup)


	def add_state(self,entity, old_state="", new_state=""):
		"""Handle the sensor state changes."""
		#_LOGGER.error("INCOMING")
		#_LOGGER.error(new_state.state)
		#_LOGGER.error(new_state.entity_id)
		#_LOGGER.error(new_state.last_changed)
		try:
			if(new_state.state=="unknown"):
				return
			try:
				test = float(new_state.state)
			except:
				return

			# fix missing timezones
			if(new_state.last_changed.tzinfo == None):
				new_state.last_changed = new_state.last_changed.replace(tzinfo=timezone('UTC'))
			now = new_state.last_changed

			v = 0
			w = self.energy_calc['extra']
			self._sample += 1

			# decide if the is grid or solar
			if(new_state.entity_id == self._gen):

				#_LOGGER.error("GEN")
				v = self.energy_calc['extra']['generator_w']
				# if this is the very first dataset, lets set the timedelta to 0, so it won't have an effect but set all parameter for the next round
				if(w['last_updated_gen']==None):
					time_delta = 0
				else:
					time_delta = (now-w['last_updated_gen']).total_seconds()

				# check if the data are somewhat strange
				if(time_delta>600):
					_LOGGER.error("warning: big time gap of "+str(time_delta)+" sec, on sample "+str(self._sample)+" now:"+str(now)+" and last_updated_gen "+str(w['last_updated_gen']))
					_LOGGER.error("the power value was "+str(v)+". This sample will be ignored by setting the time_delta to 0")
					time_delta = 0

				generated_ws = v*time_delta
				generated_kwh = generated_ws/(60*60*1000)
				self.energy_calc['extra']['generated_kwh'] += generated_kwh
				self.energy_calc['extra']['last_updated_gen'] = now
				self.energy_calc['extra']['generator_w'] = float(new_state.state)
			elif(new_state.entity_id == self._net):
				#_LOGGER.error("NET")

				# reset data when day changes, we're using NET here because solar won't change over night
				if(w['last_updated_net']==None or now.day != w['last_updated_net'].day):
					_LOGGER.error("reset data, due to day change")
					_LOGGER.error("Sample "+str(self._sample)+" old was "+str(now)+" and last_update_net "+str(w['last_updated_net']))
					self.energy_calc['extra']['generated_kwh'] = 0
					self.energy_calc['extra']['feed_in_kwh'] = 0
					self.energy_calc['extra']['feed_out_kwh'] = 0
					self.energy_calc['extra']['self_consumed_kwh'] = 0
					self.energy_calc['extra']['total_consumed_kwh'] = 0

				############ calc ################
				v = self.energy_calc['extra']['net_w']
				# if this is the very first dataset, lets set the timedelta to 0, so it won't have an effect but set all parameter for the next round
				if(w['last_updated_net']==None):
					time_delta = 0
				else:
					time_delta = (now-w['last_updated_net']).total_seconds()

				# check if the data are somewhat strange
				if(time_delta>600):
					_LOGGER.error("warning: big time gap of "+str(time_delta)+" sec, on sample "+str(self._sample)+" now:"+str(now)+" and last_updated_net "+str(w['last_updated_net']))
					_LOGGER.error("the power value was "+str(v)+". This sample will be ignored by setting the time_delta to 0")
					time_delta = 0

				self.energy_calc['extra']['net_w'] = float(new_state.state)

				if(v>0):
					#_LOGGER.debug("Feed_out (buy) %i", v)
					v_ws = v*time_delta
					v_kwh = v_ws/(60*60*1000)
					self.energy_calc['extra']['feed_out_kwh'] += v_kwh

					#gen: 800W, feed_in -123W, last update 2 sec ago: 800W*2sec self consumed
					consume_ws = (w['generator_w'])*time_delta
					consume_kwh = consume_ws/(60*60*1000)
					self.energy_calc['extra']['self_consumed_kwh'] += consume_kwh
				elif(v<0):
					va = v*(-1)
					#_LOGGER.debug("Feed_in (sell) %i", va)
					va_ws = va*time_delta
					va_kwh = va_ws/(60*60*1000)
					self.energy_calc['extra']['feed_in_kwh'] += va_kwh

					#gen: 800W, feed_in 400W, last update 2 sec ago: 400W*2sec self consumed
					consume_ws = (w['generator_w']-va)*time_delta
					consume_kwh = consume_ws/(60*60*1000)
					self.energy_calc['extra']['self_consumed_kwh'] += consume_kwh

				# calc percentage useage
				if(self.energy_calc['extra']['generated_kwh']!=0):
					self.energy_calc['self_consumed_per'] = self.energy_calc['extra']['self_consumed_kwh']/self.energy_calc['extra']['generated_kwh']*100
				else:
					self.energy_calc['self_consumed_per'] = 0

				# update time / total and current home usage
				self.energy_calc['extra']['last_updated_net'] = now
				self.energy_calc['extra']['total_consumed_kwh'] = w['generated_kwh'] - w['feed_in_kwh'] + w['feed_out_kwh']
				if(self.energy_calc['extra']['self_consumed_kwh']!=0):
					self.energy_calc['extra']['home_from_solar_per'] = self.energy_calc['extra']['self_consumed_kwh']/self.energy_calc['extra']['total_consumed_kwh']*100
				else:
					self.energy_calc['extra']['home_from_solar_per'] = 0

				self.energy_calc['extra']['home_w'] = self.energy_calc['extra']['generator_w'] + self.energy_calc['extra']['net_w']


			self.async_schedule_update_ha_state(True)

		except:
			_LOGGER.error("crash on calculation")
			if(not(self._crashed)):
				self.exc()
				self._crashed = True
		############ calc ################


	@property
	def name(self):
		"""Return the name of the sensor."""
		return self._name

	@property
	def device_state_attributes(self):
		"""Return the state attributes."""
		return self._state_attributes

	@property
	def unit_of_measurement(self):
		"""Return the unit the value is expressed in."""
		return "%"

	@property
	def state(self):
		"""Return the state of the sensor."""
		return min(round(self._state,1),100.0)

	@property
	def icon(self):
		"""Return the icon to use in the frontend."""
		return self._icon

	def exc(self):
		"""Print nicely formated exception."""
		_LOGGER.error("\n\n============= energy_calc Integration Error ================")
		_LOGGER.error("unfortunately energy_calc hit an error, please open a ticket at")
		_LOGGER.error("https://github.com/KoljaWindeler/energy_calc/issues")
		_LOGGER.error("and paste the following output:\n")
		_LOGGER.error(traceback.format_exc())
		_LOGGER.error("\nthanks, Kolja")
		_LOGGER.error("============= energy_calc Integration Error ================\n\n")


	async def async_update(self):
		"""Fetch new state data for the sensor.
		This is the only method that should fetch new data for Home Assistant.
		"""
		try:
			# update states
			w = self.energy_calc['extra']
			date_solar = ""
			date_net = ""
			try:
				fmt = "%Y-%m-%d %H:%M:%S"
#				date_solar = w['last_updated_gen'].replace(tzinfo=timezone('UTC')).astimezone(get_localzone()).strftime(fmt)
#				date_net = w['last_updated_net'].replace(tzinfo=timezone('UTC')).astimezone(get_localzone()).strftime(fmt)
				date_solar = w['last_updated_gen'].astimezone(get_localzone()).strftime(fmt)
				date_net = w['last_updated_net'].astimezone(get_localzone()).strftime(fmt)
			except:
				pass
			self._state_attributes = {
				'Solar_[W]': round(w['generator_w'],2),
				'Solar_generated_[kWh]': round(w['generated_kwh'],2),
				'Solar_to_home_[kWh]': round(w['self_consumed_kwh'],2),
				'Solar_to_home_[%]': min(100,round(self.energy_calc['self_consumed_per'],2)),
				'Solar_to_grid_[kWh]': round(w['feed_in_kwh'],2),
				'Grid_[W]' : round(w['net_w'],2),
				'Grid_to_home_[kWh]': round(w['feed_out_kwh'],2),
				'Home_[W]' : round(w['home_w'],2),
				'Home_total_consumed_[kWh]': round(w['total_consumed_kwh'],2),
				'Home_from_Solar_[%]': min(100,round(w['home_from_solar_per'],2)),
				'Solar_last_updated': date_solar,
				'Grid_last_updated': date_net
			}

			self._state = self.energy_calc['self_consumed_per']
		except Exception:
			self._state = "error"
			self.exc()



	async def _async_initialize_from_database(self):
		"""Initialize the list of states from the database.
		The query will get the list of states in DESCENDING order so that we
		can limit the result to self._sample_size. Afterwards reverse the
		list so that we get it in the right order again.
		If MaxAge is provided then query will restrict to entries younger then
		current datetime - MaxAge.
		"""
		# limit range
		records_older_then = datetime.datetime.now(get_localzone()).replace(microsecond=0, second=0, minute=0, hour=0)
		#_LOGGER.error("DB time limit:")
		#_LOGGER.error(records_older_then)

		with session_scope(hass=self.hass) as session:

			# grab grid data
			query = session.query(States).filter(States.entity_id == self._net)
			query = query.filter(States.created >= records_older_then)
			states_net = execute(query)

			# grab solar data
			query = session.query(States).filter(States.entity_id == self._gen)
			query = query.filter(States.created >= records_older_then)
			states_gen = execute(query)

			# merge and sort by date
			states = states_net + states_gen
			_LOGGER.error(states[0].last_updated)


			states.sort(key=lambda x: x.last_updated)

			#_LOGGER.error(str(len(states))+" entries found in db")
			session.expunge_all()

		for state in states:
			#all should be older based on the filter .. but we've seen strange behavior
			#if(state.last_updated > records_older_then):
			self.add_state(entity="", new_state=state)
			#else:
			#	_LOGGER.error("strange:"+str(state.last_updated))
		#_LOGGER.error("db done")
