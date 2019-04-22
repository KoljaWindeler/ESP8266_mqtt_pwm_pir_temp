### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### museum flur
    name: "dev10"
    state_topic: "dev10/r/light"
    command_topic: "dev10/s/light"
    qos: 0
    retain: True
    payload_on: "ON"
    payload_off: "OFF"

# -----------------------------------------------

homeassistant:
  customize:
    light.dev10:
      friendly_name: "Entrance Support"
      emulated_hue_name: "Kueche"
    sensor.dev10_temperature:
      friendly_name: "10 Entrance Support"
      icon: "mdi:thermometer"
    binary_sensor.dev10_motion:
      friendly_name: "10 Entrance Support"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev10"
    fname: "10 Entrance Support"

  - platform: mqtt
    state_topic: "dev10/r/temperature"
    name: "dev10_temperature"
    unit_of_measurement: "ºC"

# -----------------------------------------------

binary_sensor:
  - platform: mqtt
    state_topic: "dev10/r/motion"
    name: "dev10_motion"
    device_class: motion

# -----------------------------------------------

