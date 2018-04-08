### ball living room
# SL, DS18B20
# -----------------------------------------------
light:
  - platform: mqtt ### switch for dimmer bedroom
    name: "dev20"
    state_topic: "dev20/r/light"
    command_topic: "dev20/s/light/dimm"
    brightness_state_topic: 'dev20/r/light/brightness'
    brightness_command_topic: 'dev20/s/light/brightness'
    retain: True
    payload_on: "ON"
    payload_off: "OFF"
    brightness_scale: 99


homeassistant:
  customize:
    light.dev20:
      friendly_name: "20 Bedroom"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev20"
    fname: "20 Bedroom"

# -----------------------------------------------


