"""
A component which allows you to parse an RSS feed into a sensor

For more details about this component, please refer to the documentation at
https://github.com/custom-components/sensor.loadExternal

Following spec from https://validator.w3.org/feed/docs/rss2.html
"""
import re
import logging
import voluptuous as vol
from datetime import timedelta
from dateutil import parser
from time import strftime
from subprocess import check_output
from homeassistant.helpers.entity import Entity
import homeassistant.helpers.config_validation as cv
from homeassistant.components.sensor import (PLATFORM_SCHEMA)
from homeassistant.const import (CONF_NAME)

__version__ = '0.0.1'
_LOGGER = logging.getLogger(__name__)

REQUIREMENTS = []

CONF_FEED_PATH = 'feed_path'

DEFAULT_SCAN_INTERVAL = timedelta(hours=1)

COMPONENT_REPO = 'https://github.com/custom-components/sensor.loadExternal/'
SCAN_INTERVAL = timedelta(hours=1)
ICON = 'mdi:rss'

PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend({
    vol.Required(CONF_NAME): cv.string,
    vol.Required(CONF_FEED_PATH): cv.string,
})

def setup_platform(hass, config, add_devices, discovery_info=None):
    add_devices([loadExternalSensor(hass, config)])

class loadExternalSensor(Entity):
    def __init__(self, hass, config):
        self.hass = hass
        self._feed = config[CONF_FEED_PATH]
        self._name = config[CONF_NAME]
        self._state = None
        self._entries = []
        self.update()

    def update(self):
        self._entries = []
        cnt = 0
        d_old = ""
        if(1): #try:
           with open(self._feed) as fp:
              line = fp.readline().replace('\r','').replace('\n','')
              cnt = 1
              while line:
                 cnt += 1
                 d = line.split(',')
                 if(d_old!=""):
                    if(len(d_old)>1 and len(d)>1):
                       d.append(str(int((float(d[1])-float(d_old[1]))*1000)/1000))
                 else:
                    d.append("0")
                 d_old=d
                 d_new = [d[0], d[2]]
                 line=','.join(d_new)
                 self._entries.append(line)
                 line = fp.readline().replace('\r','').replace('\n','')
#        except:
#           pass
        self._state = cnt

    @property
    def name(self):
        return self._name

    @property
    def state(self):
        return self._state

    @property
    def icon(self):
        return ICON

    @property
    def device_state_attributes(self):
        return {
            'entries': self._entries
        }
