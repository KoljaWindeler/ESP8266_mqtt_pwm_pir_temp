# run this via /etc/rc.local
# /path/to/start_ha.sh&
sudo screen -S "HA" -d -m
sudo screen -r "HA" -X stuff "hass\n"
sudo screen -S "NR" -d -m
sudo screen -r "NR" -X stuff "node-red\n"
