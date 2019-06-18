# run this via /etc/rc.local
# /path/to/start_ha.sh&
sudo screen -S "HA" -d -m
sudo screen -r "HA" -X stuff "su -s /bin/bash homeassistant -c 'whoami; source /srv/homeassistant/bin/activate; pip3 install --upgrade homeassistant'\n"
