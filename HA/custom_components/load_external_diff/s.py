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


__version__ = '0.0.1'
_LOGGER = logging.getLogger(__name__)

REQUIREMENTS = []

CONF_FEED_PATH = 'feed_path'

DEFAULT_SCAN_INTERVAL = timedelta(hours=1)

COMPONENT_REPO = 'https://github.com/custom-components/sensor.loadExternal/'
SCAN_INTERVAL = timedelta(hours=1)
ICON = 'mdi:rss'

with open("/root/h/longterm_logs/counter_total.csv") as fp:
   line = fp.readline()
   cnt = 1
   d_old = ""
   while line:
      line = fp.readline().replace('\r','').replace('\n','')
      cnt += 1
      d = line.split(',')
      if(d_old!=""):
         if(len(d_old)>1 and len(d)>1):
            d.append(str(int((float(d[1])-float(d_old[1]))*1000)/1000))
      else:
         d.append("0")
      d_old=d

      line=','.join(d)
      print(line)
