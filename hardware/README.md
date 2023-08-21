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
- **Gerbers:** Ready to build design files for the programmer.

The hardware design is created using industry-standard design software, and you are encouraged to review, modify, or contribute to the design according to your needs.

## __Design Updates to the Hardware/Contributing__
Starting with version 1.0.0 I have tried not to have any breaking changes which should be very easy given such simple hardware. 
If you want to contribute and make any changes to this repo please ensure that you don't break any backwards compatibility to the hardware unless you plan on forking it and making it your own. When making non-breaking minor changes to the design please use a branch with the next minor version(1.+.0) or next patch version(1.x.+) and update the schematic and pcb accordingly.

## __Building the Hardware in Kicad__
When generating the appropriate gerber and drill files they should go in the [gerbers](./gerbers/) folder. Any new version should provide a zipped in same folder containing all the build files and replacing the old version. This should make it easy to upload the design to the pcb manufacturers. The defaults in Kicad should befine to generate gerbers and drill files only the output directory should have to be changed.

## __Assembling the PCB__
There are a very low number of components on the board and assembly is straight forward. One of the biggest options you have is the type of socket used. The BOM includes a Universal ZIF Socket(U1) that allows for plugging in both narrow and wide ICs, however a standard 28-Pin Wide DIP Socket(U1) can be used if you only need to program wide chips or an 8-Pin DIP Socket(U2) can be used for small serial EEPROMs. You can use either or, with a little encouragement, have both sockets in place at the same time. This is much cheaper than using the ZIF sockets but not as elegant(or as good for the IC Pins). 

J2 is a 1x3 Pin Header meant to be used with a shunt jumper to select between the different types of EEPROMs, 24-Pin or 28-Pin Parallel EEPROMS or 8 Pin Serial EEPROMs. You can also install a small SPDT sliding switch in place of the Pin Headers and the serial programming will work in either position.

J1 shuold be right angle 40-Pin connector. If you use a standard vertical 40-Pin Connector it will orient the EEPROM Programmer vertical/perpendicular to a Pi 400 and parallel and updside down to a standard Pi 4.

While I've included sources for all the parts feel free to use your own or source them from wherever you like or can find them cheapest. I've used a number of combinations of different components in testing and they've all worked fine.

Start by install the Sockets(U1/U2)(smallest first if you are trying to use both). Then install the Pin Header(JP2) and add your jumper shunt. Finally, install the right angle 40-Pin Connector.

## __Installation/Usage__
The Pi EEPROM Programmer should plug right into the GPIO Pins of the Raspberry Pi 4 or 400. Take note to make sure the pins are aligned correctly if you opt not to use a keyed 40-Pin connector.

For the Pi 400 the Programmer should stick out of the back of the case and for a Pi 4 the programmer should be vertical straight up assuming you are using the standard components and not others mentioned above.

You can then use the software provided in this repositor to read and write EEPROMs.

## __Features, Bugs, and Contributing__
If there's anything you'd like to add or find a bug please open an [issue](https://github.com/andrewteall/pi-eeprom-programmer/issues). If you'd like to implement those chages yourself please feel free to open a PR or fork the repository and make it your own.


## __License__
This project is licensed under the [MIT License](../LICENSE). You are free to use, modify, and distribute the project according to the terms of the license.
