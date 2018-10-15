"""
Support for JKW MQTT sensors.

For more details about this platform, please refer to the documentation at
https://home-assistant.io/components/sensor.mqtt/
"""
import asyncio
import logging
import time

from datetime import timedelta
import voluptuous as vol
from homeassistant.core import callback
from homeassistant.const import (CONF_NAME, STATE_UNKNOWN, ATTR_FRIENDLY_NAME)
from homeassistant.helpers.entity import Entity
import homeassistant.components.mqtt as mqtt
import homeassistant.helpers.config_validation as cv
from homeassistant.helpers.event import async_track_point_in_utc_time
from homeassistant.helpers.entity import Entity, async_generate_entity_id
from homeassistant.util import dt as dt_util
from homeassistant.helpers.typing import HomeAssistantType
from homeassistant.components.sensor import PLATFORM_SCHEMA, ENTITY_ID_FORMAT


_LOGGER = logging.getLogger(__name__)
DEFAULT_NAME = 'MQTT Sensor'
DEPENDENCIES = ['mqtt']

#PLATFORM_SCHEMA = mqtt.MQTT_RO_PLATFORM_SCHEMA.extend({
#    vol.Optional(CONF_NAME, default=DEFAULT_NAME): cv.string,
#})


@asyncio.coroutine
def async_setup_platform(hass, config, async_add_devices, discovery_info=None):
    """Set up MQTT Sensor."""
    if discovery_info is not None:
        config = PLATFORM_SCHEMA(discovery_info)

    name      = config.get(CONF_NAME)
    friendly  = config.get("fname")
    qos       = 0
    no_update = 0
    update    = 1

    async_add_devices([
    # entries RSSI
      JkwMqttSensor(hass, name+"_rssi",name+"/r/rssi",qos,'%',no_update,"mdi:signal-variant",friendly+" Signal"),
    # entries BSSID
      JkwMqttSensor(hass, name+"_bssid",name+"/r/BSSID",qos,'',no_update,"mdi:information-outline",friendly),
    # entries SSID
      JkwMqttSensor(hass, name+"_ssid",name+"/r/SSID",qos,'',no_update,"mdi:information-outline",friendly),
    # entries update
      JkwMqttSensor(hass, name+"_info",name+"/r/INFO",qos,'',no_update,"mdi:information-outline",friendly),
    # entries capability
      JkwMqttSensor(hass, name+"_capability",name+"/r/capability",qos,'',no_update,"mdi:information-outline",friendly),
    # entries update
      JkwMqttSensor(hass, name+"_update",name+"/r/#",qos,'min',update,"mdi:timer-sand",friendly),
    # entries all
    #  JkwMqttSensor(hass, name,name+"/r/#",qos,'',no_update,"mdi:information-outline",friendly),
    # end of entries
    ])


class JkwMqttSensor(Entity):
    """Representation of a sensor that can be updated using MQTT."""

    def __init__(self,hass: HomeAssistantType, name, state_topic, qos, unit_of_measurement,
                 update,icon,fname):
        """Initialize the sensor."""
        self._state = STATE_UNKNOWN
        self._name = name
        self._fname = fname
        self._state_topic = state_topic
        self._qos = qos
        self._icon = icon
        self._unit_of_measurement = unit_of_measurement
        self._update = update
        self._last_update = time.time()
        self.entity_id = async_generate_entity_id(ENTITY_ID_FORMAT, name, hass=hass)

    def async_added_to_hass(self):
        """Subscribe to MQTT events.
        This method must be run in the event loop and returns a coroutine.
        """
        @callback
        def message_received(topic, payload, qos):
            """Handle new MQTT messages."""
            self._last_update = time.time()
            # set time to zero if this is a update unit 
            if(self._update):
               self._state = int((time.time()-self._last_update)/60)
            else:
               self._state = payload
            self.async_schedule_update_ha_state()

        # run update all 30 sec 
        if(self._update):
           expiration_at = (dt_util.utcnow() + timedelta(seconds=30))
           async_track_point_in_utc_time(self.hass, self.update, expiration_at)

        return mqtt.async_subscribe(self.hass, self._state_topic, message_received, self._qos)

    @property
    def should_poll(self):
        """No polling needed."""
        return False

    @property
    def name(self):
        """Return the name of the sensor."""
        return self._fname

    @property
    def icon(self):
        """Return the mdi icon of the sensor."""
        return self._icon

    @property
    def unit_of_measurement(self):
        """Return the unit this state is expressed in."""
        return self._unit_of_measurement

    @property
    def state(self):
        """Return the state of the entity."""
        return self._state


    def update(self, second_arg):
        """ this will be callen every 30 sec """
        self._state = int((time.time()-self._last_update)/60)
        self.async_schedule_update_ha_state()

        # run update all 30 sec 
        if(self._update):
           expiration_at = (dt_util.utcnow() + timedelta(seconds=30))
           async_track_point_in_utc_time(self.hass, self.update, expiration_at)
