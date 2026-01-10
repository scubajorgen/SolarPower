# The SolarPower Project
## Introduction
The goal of this project is to monitor the PV system (photovoltaic system or solar system) and monitor energy usage. The project consists of two applications:

* The **solarserver**  
  This is an application for reading out pulse meters and the NTA8130 Dutch smart meter it stores measurement values in memory for read out by the solarclient application. It also offers the feature of publishing real-time measurements. It is intended for the Raspberry Pi. 
* The **solarclient**  
  This application connects to the solarserver and downloads the measurement data and stores it in a mysql database. From here, it can be presented e.g. by means of a website or application.

Dependencies are towards [libsockets](https://libwebsockets.org/), [wiringPi](https://github.com/WiringPi/WiringPi) and [rabbitmq-c](https://github.com/alanxz/rabbitmq-c).


During the project the Raspberry Pi Model 1 B+ was used. This page assumes this device.

# Installation
## Preparing the Raspberry Pi and building solarserver

* Install *Rasberry Pi OS Lite (32-bit)*, image  to a flash card, e.g. using Raspberry Pi Imager
* Get the Raspberry Pi up and running.
* Make sure Serial Port 0 (/dev/ttyAMA0) is not used as terminal. Use **raspi-config**:  
  5 Interfacing Options -> P6 Serial -> 'Would you like a login shell to be accessible over serial?' = no
  ```
  sudo raspi-config
  >  3 Interface Options
  >  I6 Serial Port
  >  Would you like a login shell to be accessible over serial? <No>
  >  Would you like the serial port hardware to be enabled? <Yes>
  > <Ok>
  > <Finish>
  ```
* Update:  ```sudo apt-get update```
* Install libwebsockets, if you are going to use websockets:  
  ```
    sudo apt-get install libwebsockets-dev
  ```
* Install wiringPi  
  Get the latest armhf release from the [WiringPi github](https://github.com/WiringPi/WiringPi/releases), at time of writing it was 3.16
  ```
  cd
  wget https://github.com/WiringPi/WiringPi/releases/download/3.16/wiringpi_3.16_armhf.deb
  sudo dpkg -i wiringpi_3.16_armhf.deb
  ```
* If you are going to use AMQP (e.g. RabbitMQ), download and install [rabbitmq-c](https://github.com/alanxz/rabbitmq-c), latest release (at time of writing 0.15.0)  
  ```
    wget https://github.com/alanxz/rabbitmq-c/releases/tag/v0.15.0
    tar -xzvf rabbitmq-c-0.15.0.tar.gz
    cd rabbitmq-c-0.15.0/
    mkdir build
    cd build
    cmake ..
    cmake --build .
    sudo make install
  ```
* Copy sources of solarserver using samba, scp, or whatever
* Build:  
  ```
  make clean
  make
  ```
* Create or adapt config.ini
* Run  
  ```
  ./Solar
  ```

Optional, modify according to own flavour:
* Install Joe's Own Editor (joe)
  ```
  sudo apt-get install joe
  ```
* [Install Samba](https://raspberrypi.tilburgs.com/samba/)  
  ```
  sudo apt-get install samba samba-common-bin
  sudo smbpasswd -a [user]
  ```
  Add to /etc/samba/smb.conf
  ```
  workgroup = your_workgroup_name
  wins support = yes
  ...
  [userhome]
   comment = User Home
   path = /home/%U
   public = no
   writable = yes
  ```
  Restart
  ```
  sudo service samba restart
  ```
* Install screen  
  ```
  sudo apt-get install screen
  ```

## Building solarclient
Run the software on a Linux machine or server; it might even work under Windows using g++ and MinGW.
* Building
  ```
  make clean
  make
  ```
* Create MySQL database using the script createdb.sql from within mysql
  ```
  source createdb.sql
  ```
  Grant privileges to a user
* Add or modify config.ini
  Add the database, database credentials, solarserver address, etc
* Run
  ```
  ./SolarClient
  ```


# SERIAL PORT TEST

sudo apt-get install python-serial

python p1.py

Note that some meters have inverted signals.

# CONFIGURATION
Configuration is in config.ini. All values must be set and must
be valid!