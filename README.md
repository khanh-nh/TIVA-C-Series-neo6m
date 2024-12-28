# TIVA-C-Series-neo6m
This project implements the GPS module Ublox Neo 6m using the Tiva C series TM4C123G launchpad. 
The system will receive the data from the Neo 6m module, decode into longitude and latitude, then use it to calculate the distance continuousely. When the distance exceed the desired distance, the buzzer will be triggered.

## Components List
1. Tiva C series TM4C123G Launchpad.
2. Ublox Neo-6m GPS module. 
3. 16x2 LCD.
4. I2C module for 16x2 LCD.
5. Buzzer

## Pin map: 

Buzzer (+) -> PF3\
Buzzer (-) -> GND\
LCD (+) -> VBUS\
LCD (-) -> GND\
LCD (SDA) -> PB1\
LCD (SCL) -> PB0\
GPS (TX) -> PB0 (RX)\
GPS (+) -> 3.3V\
GPS (-) -> GND

**Contacts:** \
22khanh.nh@vinuni.edu.vn \
22dan.bd@vinuni.edu.vn
