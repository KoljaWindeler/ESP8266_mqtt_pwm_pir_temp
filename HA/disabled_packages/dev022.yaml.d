### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### xmas
    name: "dev22"
    state_topic: "dev22/r/light"
    command_topic: "dev22/s/light"
    retain: True
    qos: 0
    payload_on: "ON"
    payload_off: "OFF"
    optimistic: false

# -----------------------------------------------

homeassistant:
  customize:
    light.dev22:
      friendly_name: "22 xmas"
    binary_sensor.dev22_motion:
      friendly_name: "22 xmas"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev22"
    fname: "22 xmas"

# -----------------------------------------------

binary_sensor:
  - platform: mqtt
    state_topic: "dev22/r/motion"
    name: "dev22_motion"
    device_class: motion

# -----------------------------------------------

