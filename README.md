# PID Controlled Heater Project

This repository holds the files for a PID Controlled Heater project that I created for my Bachelor of Engineering qualification.

For this project, I was provided a 230V 1.5KW heating element then tasked with building a heater around it. I had to create a heater that heat the air up to and maintain it at a user defined temperature. I accomplished this by creating a PCB that used an ESP32-C3 microcontroller to infer temperature from the voltage drop across a thermistor and control the power level of the heating element and a fan.

## Microcontroller source code
The "Microcontroller_Source" folder contains the microcontroller's firmware. It was written in C++ and targets the Arduino API. PLatformIO is used to build and flash the firmware onto the microcontroller. Please see [the installation instructions](https://platformio.org/platformio-ide) for getting it working in your IDE.
