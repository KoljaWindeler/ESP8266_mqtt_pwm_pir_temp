### ball living room
# SL, DS18B20
# -----------------------------------------------

homeassistant:
  customize:
    sensor.dev8_temperature:
      friendly_name: "Carlo PIR"
      icon: "mdi:thermometer"
    binary_sensor.dev8_motion:
      friendly_name: "Carlo PIR"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev8"
    fname: "08 CarloPIR"

# -----------------------------------------------

binary_sensor:
  - platform: mqtt
    state_topic: "dev8/r/motion"
    name: "dev8_motion"
    device_class: motion

# -----------------------------------------------

