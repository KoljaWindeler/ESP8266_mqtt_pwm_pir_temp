# run this via /etc/rc.local
# /path/to/start_ha.sh&
exit 0;
sudo screen -S "AD" -d -m
sudo screen -r "AD" -X stuff "appdaemon -c /home/homeassistant/.homeassistant/appdaemon\n"
