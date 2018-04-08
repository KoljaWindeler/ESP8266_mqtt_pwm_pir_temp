### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### palette
    name: "dev22"
    state_topic: "dev22/r/light"
    command_topic: "dev22/s/light/dimm"
    brightness_state_topic: 'dev22/r/light/brightness'
    brightness_command_topic: 'dev22/s/light/brightness'
    retain: True
    qos: 0
    payload_on: "ON"
    payload_off: "OFF"
    optimistic: false
    brightness_scale: 99

# -----------------------------------------------

homeassistant:
  customize:
    light.dev22:
      friendly_name: "22 Palette"
    binary_sensor.dev22_motion:
      friendly_name: "22 Palette"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev22"
    fname: "22 Palette"

# -----------------------------------------------

binary_sensor:
  - platform: mqtt
    state_topic: "dev22/r/motion"
    name: "dev22_motion"
    device_class: motion

# -----------------------------------------------

