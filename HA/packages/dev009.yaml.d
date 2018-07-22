### ball living room
# SL, DS18B20
# -----------------------------------------------

light:
  - platform: mqtt ### vac
    name: "dev9"
    state_topic: "dev9/r/light"
    command_topic: "dev9/s/light"
    qos: 0
    retain: True
    payload_on: "ON"
    payload_off: "OFF"

# -----------------------------------------------

homeassistant:
  customize:
    light.dev9:
      friendly_name: "Vac"
    sensor.dev9_temperature:
      friendly_name: "Shop VAC"
      icon: "mdi:thermometer"

# -----------------------------------------------

sensor:
  - platform: mqtt_jkw
    name: "dev9"
    fname: "09 VAC"

  - platform: mqtt
    state_topic: "dev9/r/temperature"
    name: "dev9_temperature"
    unit_of_measurement: "ºC"

  - platform: mqtt
    state_topic: "dev9/humidity"
    name: "dev9_humidity"
    unit_of_measurement: "%"

# -----------------------------------------------

