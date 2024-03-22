![AgOpenGPS](https://github.com/m-elias/AOG-AiO-RVC-100hz/blob/main/media/agopengps%20name%20logo.png)
[AOG Download](https://github.com/farmerbriantee/AgOpenGPS/releases)<br>
[AOG Forum](https://discourse.agopengps.com/)<br>
[AOG YouTube](https://youtube.com/@AgOpenGPS)

## UDP Machine class
An attempt to put all [AgOpenGPS](https://github.com/farmerbriantee/AgOpenGPS/releases) Machine/Section control related code into an Arduino class/library for UDP communications.

### Source & Examples
See the machine.h file in the two example sketches for Teensy 4.1 (Eth) and Nano (enc28j60) implementation.

The Teensy example includes code for using PCA9555 (I2C expander) outputs.

### To do:
- ESP32 example
- init() function for section only control
- customizable number of PCA9555 outputs
- Add section "switch" control PGN

### AgOpenGPS Machine Configuration
Use the Machine settings in AgOpenGPS to set the machine config settings.
![machine config](https://github.com/m-elias/AOG-Machine/blob/main/media/aog%20machine%20config.jpg)

Use the Pin config settings to set the function for each output pin.
![pin config](https://github.com/m-elias/AOG-Machine/blob/main/media/aog%20pin%20config.jpg)
