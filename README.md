![AgOpenGPS](https://github.com/m-elias/AOG-AiO-RVC-100hz/blob/main/media/agopengps%20name%20logo.png)
[AOG Download](https://github.com/farmerbriantee/AgOpenGPS/releases)<br>
[AOG Forum](https://discourse.agopengps.com/)<br>
[AOG YouTube](https://youtube.com/@AgOpenGPS)

## UDP Machine class
An attempt to put all [AgOpenGPS](https://github.com/farmerbriantee/AgOpenGPS/releases) Machine/Section control related code into an Arduino class/library for UDP communications.

### Source & Examples
See the machine.h file in the example sketches for Teensy 4.1 (Eth), Nano (enc28j60) & ESP32 (Wifi) implementation.
- note the ESP32 example has a new re-worked machine class with callback functions
- eventually the Teensy/Nano examples will follow suit

The Teensy example includes code for using PCA9555 (I2C expander) outputs.

### To do:
- Consolidate Teensy/Nano examples to use same (newer) ESP32 machine class
- Add section "switch" control PGN?
  - maybe in it's own class

### AgOpenGPS Machine Configuration
Use the Machine settings in AgOpenGPS to set the machine config settings.
![machine config](https://github.com/m-elias/AOG-Machine/blob/main/media/aog%20machine%20config.jpg)

Use the Pin config settings to set the function for each output pin.
![pin config](https://github.com/m-elias/AOG-Machine/blob/main/media/aog%20pin%20config.jpg)
