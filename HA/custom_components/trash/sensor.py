"""
@ Author      : Daniel Palstra & Bram van Dartel
@ Date        : 06/12/2017
@ Description : MijnAfvalwijzer Sensor - It queries mijnafvalwijzer.nl.
@ Notes:        Copy this file and place it in your
                "Home Assistant Config folder\custom_components\sensor\" folder.
"""
import asyncio

import homeassistant.helpers.config_validation as cv
from homeassistant.helpers.entity import Entity, async_generate_entity_id
from homeassistant.components.sensor import PLATFORM_SCHEMA, ENTITY_ID_FORMAT
from homeassistant.const import (CONF_NAME)
from homeassistant.util import Throttle
from homeassistant.helpers.typing import HomeAssistantType

from urllib.request import urlopen
from datetime import timedelta
import datetime
import logging
import unicodedata
import voluptuous as vol

_LOGGER = logging.getLogger(__name__)

ICON = 'mdi:delete'
DOMAIN = "trash"

TRASH_TYPES = [1,2,3,4]
SCAN_INTERVAL = timedelta(minutes=1)
#SCAN_INTERVAL = timedelta(seconds=15)

DEFAULT_NAME = 'Trash_Sensor'
CONST_REGIONCODE = "regioncode"

PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend({
    vol.Required(CONST_REGIONCODE): cv.string,
})

@asyncio.coroutine
def async_setup_platform(hass, config, async_add_devices, discovery_info=None):
    """Set up date afval sensor."""
    regioncode = config.get(CONST_REGIONCODE)
    #{0} and {1} will be filled later
    url1 = "http://www.zacelle.de/privatkunden/muellabfuhr/abfuhrtermine/?tx_ckcellextermine_pi1%5Bot%5D="
    url2 = "&tx_ckcellextermine_pi1%5Bics%5D="
    url3 = "&tx_ckcellextermine_pi1%5Bstartingpoint%5D="
    url4 = "&type=3333"
    data = TrashCollectionSchedule(url1,url2,url3,url4,regioncode)

    devices = []
    for trash_type_id in range(0,4): # 4 types of trash
        devices.append(TrashCollectionSensor(hass, trash_type_id, data))
    async_add_devices(devices)


class TrashCollectionSensor(Entity):
    """Representation of a Sensor."""

    def __init__(self,hass: HomeAssistantType, trash_type_id, data):
        """Initialize the sensor."""
        self._state_attributes = None
        self._state = None
        self._trash_type_id = trash_type_id
        self._name = "Loading ("+str(trash_type_id)+")"
        self.data = data
        self.entity_id = async_generate_entity_id(ENTITY_ID_FORMAT, "trash_" + str(trash_type_id), hass=hass)

    @property
    def name(self):
        """Return the name of the sensor."""
        return self._name

    @property
    def device_state_attributes(self):
        """Return the state attributes."""
        return self._state_attributes

    @property
    def state(self):
        """Return the state of the sensor."""
        return self._state

    @property
    def icon(self):
        """Return the icon to use in the frontend."""
        return ICON

    def update(self):
        """Fetch new state data for the sensor.
        This is the only method that should fetch new data for Home Assistant.
        """
        today = datetime.datetime.now()
        if self.data.f_date != str(today.strftime("%d")):
            self.data.update()
        if type(self.data.data) == list:
            if len(self.data.data[self._trash_type_id]) == 3:
                try:
                   self._state = self.data.data[self._trash_type_id]['pickup_date'] + ' (%02i)' % int(self.data.data[self._trash_type_id]['extra']['remaining'])
                except:
                   self._state = self.data.data[self._trash_type_id]['pickup_date'] + ' ('+ int(self.data.data[self._trash_type_id]['extra']['remaining'] +')'
                   self._state = self.data.data[self._trash_type_id]['pickup_date'] + ' (-)'
                self._name = self.data.data[self._trash_type_id]['name_type']
                self._state_attributes = self.data.data[self._trash_type_id]['extra']
#                print("trash sensor update failed")
#                print(self.data.data)
#                print("trash sensor update failed")


class TrashCollectionSchedule(object):

    def __init__(self, url1,url2,url3,url4,regioncode):
        self._url1 = url1
        self._url2 = url2
        self._url3 = url3
        self._url4 = url4
        self._rc = regioncode
        self.data = None
        self.f_date = 0


    @Throttle(SCAN_INTERVAL)
    def update(self):
        today = datetime.datetime.now()
        trash_data = []
        magic_nr = 234
        types_found = 0

        #print("running update")
        for type in range(0,4):
            url = self._url1+str(self._rc)+self._url2+str(type)+self._url3+str(magic_nr)+self._url4
            log = str(type)+" open url:"+url+"\r\n"
            #print(url)
            response = urlopen(url)
            string = response.read().decode('ISO-8859-1')
            log+="got response with n byte "+str(len(string))+"\r\n"
            entries = string.split("BEGIN:VEVENT")
            log+="got "+str(len(entries))+" entries\r\n"
            trash = {}
            trash['name_type'] = "-"
            trash['pickup_date'] = "-"
            extra={}
            extra['remaining'] = "-"
            trash['extra'] = extra
            try:
               for i in range(1,len(entries)):
                    log+="check entry "+str(i)+"\r\n"
                    l = entries[i].split('\n')
                    log+=str(len(l))+" lines\r\n"
                    for ii in range(1,len(l)):
                        #log+="checking line "+str(ii)+" for SUMMARY ("+l[ii]+")\r\n"
                        if(l[ii].find("SUMMARY")>=0):
                            log+="summary line found "+l[ii]+"\r\n"
                            s = l[ii].split(":")
                            if(len(s)>1):
                                s = s[1]
                    #for iii in range(0,len(s)):
                        #print(str(ii)+"/"+str(iii)+" "+str(ord(s[iii]))+" "+str(s[iii]) )
                                s=''.join(e for e in s if (e.isalnum() or e==' '))
                                s = s.replace(chr(195), 'u')
                                s = s.replace(chr(188), 'e')
                                log+="Setting name as ->"+s+"<-\r\n"
                                trash['name_type'] = s
                                extra['remaining'] = "-"
                                trash['extra'] = extra
                                trash['pickup_date'] = "-"

                    for ii in range(1,len(l)):
                        #log+="checking line "+str(ii)+" for DTSTART ("+l[ii]+")\r\n"
                        if(l[ii].find("DTSTART")>=0):
                            log+="DTSTART line found "+l[ii]+"\r\n"
                            s = l[ii].split(":")
                            if(len(s)>1):
                                s = s[1]
                                if(s.find("T")>0):
                                    s = s.split("T")[0]
                                    dd=''.join(e for e in s if (e.isalnum() or e==' '))
                                    log+="use date -->"+str(dd)+"<--\r\n"
                    d = datetime.datetime.strptime(str(dd), "%Y%m%d")
                    if d > today:
                        log+="FOUND, DONE\r\n"
                        types_found = types_found + 1
                        trash['pickup_date'] = str(d.strftime("%d.%m.%Y"))
                        rem = d - datetime.datetime.now()
                        extra['remaining'] = str(int(rem.total_seconds()/86400))
                        trash['extra'] = extra
                        break
                    else:
                        trash['pickup_date'] = "no pick up"
                        extra['remaining'] = "-"
                        trash['extra'] = extra
                        log+=" not after today, next\r\n\r\n"
            except:
                print("FAILURE: "+log+"\r\n")
            #print(log)
            trash_data.append(trash)
        self.data = trash_data
        if types_found == 4:
            self.f_date = str(today.strftime("%d"))
