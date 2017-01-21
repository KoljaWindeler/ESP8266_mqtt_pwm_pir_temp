# run this via /etc/rc.local
# /path/to/start_ha.sh&
sudo screen -S "A" -d -m
sudo screen -r "A" -X stuff "su -s /bin/bash homeassistant -c 'whoami; source /srv/homeassistant/bin/activate; hass'\n"
