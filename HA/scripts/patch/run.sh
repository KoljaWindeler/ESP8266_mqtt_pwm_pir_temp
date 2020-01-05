echo "copy manifest"
cp manifest.json /srv/homeassistant/lib/python3.7/site-packages/homeassistant/components/stream/manifest.json
echo "copy camera.py"
cp camera /srv/homeassistant/lib/python3.7/site-packages/homeassistant/components/ -r
echo "copy onvif"
cp onvif /srv/homeassistant/lib/python3.7/site-packages/homeassistant/components/ -r
echo "Done"
