### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### b1
    name: "dev12"
    state_topic: "dev12/r/light"
    command_topic: "dev12/s/light/dimm"
    rgb_state_topic: 'dev12/r/light/dimm/color'
    rgb_command_topic: 'dev12/s/light/dimm/color'
    brightness_state_topic: 'dev12/r/light/brightness'
    brightness_command_topic: 'dev12/s/light/brightness'
    retain: True
    qos: 0
    payload_on: "ON"
    payload_off: "OFF"
    optimistic: false
    brightness_scale: 99
  - platform: mqtt ### b1
    name: "dev12_simple_rainbow"
    state_topic: "dev12/r/light/animation/simple_rainbow"
    command_topic: "dev12/s/light/animation/simple_rainbow"
    retain: True
    payload_on: "ON"
    payload_off: "OFF"

# -----------------------------------------------

homeassistant:
  customize:
    light.dev12:
      friendly_name: "Entrance B1"
      icon: "mdi:lamp"
    light.dev12_simple_rainbow:
      icon: "mdi:traffic-light"
      friendly_name: "B1 Simple Rainbow"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev12"
    fname: "12 EntranceB1"

# -----------------------------------------------

binary_sensor:
  - platform: mqtt
    state_topic: "dev12/r/motion"
    name: "dev12_motion"
    device_class: motion


# -----------------------------------------------

