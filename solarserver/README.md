# INTRODUCTION
The solarserver is an application for reading out pulse meters and
the NTA8130 Dutch smart meter. It is intended for the Raspberry Pi.
Dependencies are towards libsockets and wiringPi.

Make sure Serial Port 0 (/dev/ttyAMA0) is not used as terminal. 
raspi-config -> 5 Interfacing Options -> P6 Serial -> 'Would you like a login shell to be accessible over serial?' = no

# INSTALLATION
Install rasberry pi OS Lite, image 32bit
sudo apt-get update
sudo apt-get install openvpn
Copy files to /etc/openvpn, make sure the extension is .conf instead of .ovpn
Add line to /etc/default/openvpn
sudo systemctl daemon-reload
sudo systemctl start openvpn
sudo systemctl enable openvpn

sudo apt-get install libwebsockets-dev

sudo apt-get install git-core
cd
wget https://project-downloads.drogon.net/wiringpi-latest.deb
sudo dpkg -i wiringpi-latest.deb

make clean
make

# SERIAL PORT TEST

sudo apt-get install python-serial

python p1.py

Note that some meters have inverted signals.

# CONFIGURATION
Configuration is in config.ini. All values must be set and must
be valid!