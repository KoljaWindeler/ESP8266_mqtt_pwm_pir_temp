exit 0;
sudo screen -S "HA" -d -m
sudo screen -r "HA" -X stuff "su -s /bin/bash homeassistant -c 'whoami; source /srv/homeassistant/bin/activate; hass'\n"
#sudo screen -S "AD" -d -m
#sudo screen -r "AD" -X stuff "appdaemon -c /home/homeassistant/.homeassistant/appdaemon\n"
#sudo screen -r "HA" -X stuff "su -s /bin/bash homeassistant -c 'whoami; source /srv/homeassistant/bin/activate; pip3 install --upgrade homeassistant'\n"
