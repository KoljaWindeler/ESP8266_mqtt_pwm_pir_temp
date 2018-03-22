"""
@ Author      : Daniel Palstra & Bram van Dartel
@ Date        : 06/12/2017
@ Description : MijnAfvalwijzer Sensor - It queries mijnafvalwijzer.nl.
@ Notes:        Copy this file and place it in your
                "Home Assistant Config folder\custom_components\sensor\" folder.
"""
import homeassistant.helpers.config_validation as cv
from homeassistant.helpers.entity import Entity
from homeassistant.components.sensor import PLATFORM_SCHEMA
from homeassistant.const import (CONF_NAME)
from homeassistant.util import Throttle

from urllib.request import urlopen
from datetime import timedelta
import json
import argparse
import datetime
import logging

import voluptuous as vol

_LOGGER = logging.getLogger(__name__)

ICON = 'mdi:delete-empty'

TRASH_TYPES = [1,2,3,4]
SCAN_INTERVAL = timedelta(minutes=1)
#SCAN_INTERVAL = timedelta(seconds=15)

DEFAULT_NAME = 'Trash_Sensor'
CONST_REGIONCODE = "regioncode"

#PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend({
#    vol.Required(CONST_REGIONCODE): cv.string,
#})


def async_setup_platform(hass, config, async_add_devices, discovery_info=None):
    """Set up date afval sensor."""
    regioncode = 148#config.get(CONST_REGIONCODE)
    url = "http://www.zacelle.de/privatkunden/muellabfuhr/abfuhrtermine/?tx_ckcellextermine_pi1[ot]="+regioncode+"&tx_ckcellextermine_pi1[ics]={0}&tx_ckcellextermine_pi1[startingpoint]={1}&type=3333"
    data = TrashCollectionSchedule(url)

    devices = []
    for trash_type_id in range(0,4): # 4 types of trash
        devices.append(TrashCollectionSensor(trash_type_id, data))
    async_add_devices(devices)


class TrashCollectionSensor(Entity):
    """Representation of a Sensor."""

    def __init__(self, trash_type_id, data):
        """Initialize the sensor."""
        self._state = None
        self._trash_type_id = trash_type_id
        self._name = "trash_ff_"+trash_type_id
        self.data = data
        self.entity_id = async_generate_entity_id(ENTITY_ID_FORMAT, "trash_" + trash_type_id, hass=hass)

    @property
    def name(self):
        """Return the name of the sensor."""
        return self._name

    @property
    def state(self):
        """Return the state of the sensor."""
        return self._state

    @property
    def icon(self):
        """Return the icon to use in the frontend."""
        #if self._trash_type_id == 0:
        #    return ... 
        return ICON

    def update(self):
        """Fetch new state data for the sensor.
        This is the only method that should fetch new data for Home Assistant.
        """
        self.data.update()
        self._state=self.data.data[self._trash_type_id]['pickup_date']
        self._name=self.data.data[self._trash_type_id]['name_type']


class TrashCollectionSchedule(object):

    def __init__(self, url):
        self._url = url
        self.data = None

    @Throttle(SCAN_INTERVAL)
    def update(self):
        today = datetime.datetime.now()
        trash_data = []
        magic_nr = 234

        for ii in range(0,4):
            url = self._url.format(ii,magic_nr)
            response = urlopen(url)
            string = response.read().decode('ISO-8859-1')
            entries = string.split("BEGIN:VEVENT")
            trash = {}
            for i in range(1,len(entries)):
                l = entries[i].split('\n')
                trash['name_type'] = l[4].split("SUMMARY:")[1]
                trash['pickup_date'] = "-"
                if datetime.datetime.strptime(l[2].split("DATE:")[1], "%Y%m%d") > today:
                    trash['pickup_date'] = (str(datetime.datetime.strptime(l[2].split("DATE:")[1], "%Y%m%d")))
                    trash_data.append(trash)
                    break
        
        self.data = trash_data
