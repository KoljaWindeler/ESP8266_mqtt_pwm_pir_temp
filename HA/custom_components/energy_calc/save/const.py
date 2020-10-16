from homeassistant.helpers.config_validation import entity_id
from homeassistant.components.sensor import PLATFORM_SCHEMA, ENTITY_ID_FORMAT
import homeassistant.helpers.config_validation as cv
import voluptuous as vol
import requests
import traceback
import logging
import datetime
from collections import OrderedDict


_LOGGER = logging.getLogger(__name__)


# generals
DOMAIN = "energy_calc"
PLATFORM = "sensor"
VERSION = "0.1.0"
ISSUE_URL = "https://github.com/koljawindeler/energy_calc/issues"
SCAN_INTERVAL = datetime.timedelta(seconds=10)

# configuration
CONF_ICON = "icon"
CONF_NAME = "name"
CONF_NET = "net_id"
CONF_GEN = "gen_id"

# defaults
DEFAULT_ICON = 'mdi:weather-sunny'
DEFAULT_NAME = "energy_calc"


# extend schema to load via YAML
PLATFORM_SCHEMA = PLATFORM_SCHEMA.extend({
	vol.Required(CONF_NET): cv.string,
	vol.Required(CONF_GEN): cv.string,
	vol.Optional(CONF_NAME, default=DEFAULT_NAME): cv.string,
	vol.Optional(CONF_ICON, default=DEFAULT_ICON): cv.string
})



def check_data(user_input, hass):
	"""Check validity of the provided date."""
	ret = {}
	try:
		if(not(entity_id(user_input[CONF_NET]))):
			_LOGGER.error("ICS: ID not unique")
			ret["base"] = ERROR_ID_NOT_UNIQUE
			return ret

		url = "http://"+user_input[CONF_energy_calc_URL]+"/realtime.csv"
		ret = requests.get(url).content
		return {}
	except Exception:
		ret["base"] = ERROR_URL
		return ret

def ensure_config(user_input):
	"""Make sure that needed Parameter exist and are filled with default if not."""
	out = {}
	out[CONF_NAME] = DEFAULT_NAME
	out[CONF_ICON] = DEFAULT_ICON
	out[CONF_GEN] = ""
	out[CONF_NET] = ""

	if user_input is not None:
		if CONF_NAME in user_input:
			out[CONF_NAME] = user_input[CONF_NAME]
		if CONF_ICON in user_input:
			out[CONF_ICON] = user_input[CONF_ICON]
		if CONF_GEN in user_input:
			out[CONF_GEN] = user_input[CONF_GEN]
		if CONF_NET in user_input:
			out[CONF_NET] = user_input[CONF_NET]
	return out


def create_form(user_input, hass):
	"""Create form for UI setup."""
	user_input = ensure_config(user_input, hass)

	data_schema = OrderedDict()
	data_schema[vol.Optional(CONF_NAME, default=user_input[CONF_NAME])] = str
	data_schema[vol.Optional(CONF_ICON, default=user_input[CONF_ICON])] = str
	data_schema[vol.Required(CONF_NET, default=user_input[CONF_GEN])] = str
	data_schema[vol.Required(CONF_GEN, default=user_input[CONF_NET])] = str

	return data_schema
