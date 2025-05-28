# PID Controlled Heater Project

This repository holds the files for a PID Controlled Heater project that I created for my Bachelor of Engineering qualification.

For this project, I was provided a 230V 1.5KW heating element then tasked with building a heater around it. I had to create a heater that heat the air up to and maintain it at a user defined temperature. I accomplished this by creating a PCB that used an ESP32-C3 microcontroller to infer temperature from the voltage drop across a thermistor and control the power level of the heating element and a fan.

## Microcontroller source code
The "Microcontroller_Source" folder contains the microcontroller's firmware. It was written in C++ and targets the Arduino API. PLatformIO is used to build and flash the firmware onto the microcontroller. Please see (the PlatformIO documentation)[https://docs.platformio.org/en/latest/integration/ide/index.html] for instructions to install and use it in your IDE.

## PCB files
The "PCB_Files" folder contains files related to the PCBs I created for this project. 

## Application notes
Physical modifications are required to the PCB for the Olimex display that was used in the project.

<img alt="Display jumpers" src="https://raw.githubusercontent.com/XJDHDR/PID_Controlled_Heater_Project/refs/heads/main/Olimex_Display_jumpers.png" width="210" height="323">

- To allow the backlight to be controlled, **SJ2** needs to be modified. The track between pads 1 and 2 needs to be cut, then solder needs to be added to connect pads 2 and 3. Without this change, the backlight will be constantly switched on.
- To give the touch IC the ability to signal that a touch event has occurred, **SJ3** needs to be modified. Solder needs to be added to connect the two pads. Without this change, the microcontroller has to poll the touch IC to figure out if one has occurred.
