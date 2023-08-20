# __A Raspberry Pi GPIO EEPROM Writer and Reader__ 

A way to write Parallel and Serial EEPROMs using the Raspberry Pi GPIO pins.

## __Introduction__
The Raspberry Pi EEPROM Programmer is a hardware project that facilitates the programming and updating of EEPROMs using a Raspberry Pi. Instead of having to wire a circuit up on a bread board, the idea is just to be able to plug the EEPROM in to the pcb via a socket of some sort and then plug the pcb into the Raspberry Pi's GPIO pins.

This README provides an overview of the features, hardware design, and usage instructions for the Raspberry Pi EEPROM Programmer.

## Hardware Design

The hardware design files for the Raspberry Pi EEPROM Programmer can be found in the [hardware](../hardware) directory of this repository. The design includes:

- **Kicad Project File:** The Kicad 6 project file to open the designs.
- **Schematic:** The schematic diagram detailing the connections between components and the Raspberry Pi.
- **PCB Layout:** The PCB layout design files used for creating the physical printed circuit board.
- **Bill of Materials:** A list of all components required for building the programmer.

The hardware design is created using industry-standard design software, and you are encouraged to review, modify, or contribute to the design according to your needs.

## __Design Updates to the Hardware/Contributing__
Starting with version 1.0.0 I have tried not to have any breaking changes which should be very easy given such simple hardware. 
If you want to contribute and make any changes to this repo please ensure that you don't break any backwards compatibility to the hardware unless you plan on forking it and making it your own. When making non-breaking minor changes to the design please use a branch with the next minor version(1.+.0) or next patch version(1.x.+) and update the schematic and pcb accordingly.


## __Building the Hardware in Kicad__
When generating the appropriate gerber and drill files they should go in the [gerbers](./gerbers/) folder. Any new version should provide a zipped in same folder containing all the build files and replacing the old version. This should make it easy to upload the design to the pcb manufacturers. The defaults in Kicad should befine to generate gerbers and drill files only the output directory should have to be changed.


## __Assembling the PCB__
There are a very low number of components on the board and assembly is straight forward. One of the biggest options you have is the type of socket used. The BOM includes a Universal ZIF Socket that allows for plugging in both narrow and wide ICs, however a standard 28-Pin Wide Dip Socket can be used if you only need to program wide chips. While I've included sources for all the parts feel free to use your own or source them from wherever you like or can find them cheapest. I've used a number of combinations of different components in testing and they've all worked fine.

## __Features, Bugs, and Contributing__
If there's anything you'd like to add or find a bug please open an [issue](https://github.com/andrewteall/pi-eeprom-programmer/issues). If you'd like to implement those chages yourself please feel free to open a PR or fork the repository and make it your own.


## __License__
This project is licensed under the [MIT License](../LICENSE). You are free to use, modify, and distribute the project according to the terms of the license.
