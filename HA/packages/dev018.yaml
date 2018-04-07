### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### ai light carlo
    name: "dev18"
    state_topic: "dev18/r/light"
    command_topic: "dev18/s/light/dimm"
    rgb_state_topic: 'dev18/r/light/dimm/color'
    rgb_command_topic: 'dev18/s/light/dimm/color'
    qos: 0
    retain: True
    payload_on: "ON"
    payload_off: "OFF"
    optimistic: false
    brightness_scale: 99

  - platform: mqtt 
    name: "dev18_simple_rainbow"
    state_topic: "dev18/r/light/animation/simple_rainbow"
    command_topic: "dev18/s/light/animation/simple_rainbow"
    brightness_state_topic: 'dev18/r/light/animation/brightness'
    brightness_command_topic: 'dev18/s/light/animation/brightness'
    brightness_scale: 99
    retain: True
    payload_on: "ON"
    payload_off: "OFF"

# -----------------------------------------------

homeassistant:
  customize:
    light.dev18:
      friendly_name: "18 Carlo AI"
    light.dev18_simple_rainbow:
      friendly_name: "18 Carlo AI Rainbow"
    binary_sensor.dev18_motion:
      friendly_name: "Carlo AI"
    sensor.dev18_temperature:
      friendly_name: "Carlo AI"
      icon: "mdi:thermometer"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev18"
    fname: "18 Carlo AI"

# -----------------------------------------------


