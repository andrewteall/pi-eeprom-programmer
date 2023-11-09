# __A Raspberry Pi GPIO EEPROM Writer and Reader__ 

A way to write Parallel and Serial EEPROMs using the Raspberry Pi GPIO pins.

This repository contains the software component for the Raspberry Pi EEPROM Programmer. The software is designed to work in conjunction with the hardware components to enable programming and updating of EEPROMs (Electrically Erasable Programmable Read-Only Memory) using a Raspberry Pi or the software can be used with a breadboard with the appropriate circuit wired up.

## __Introduction__

The Raspberry Pi EEPROM Programmer software provides a user-friendly interface to interact with the hardware and perform programming tasks on EEPROM chips.

This README provides an overview of the features, installation instructions, and usage guidelines for the Raspberry Pi EEPROM Programmer software.

## __Building__
See [Quick Start](../README.md#quick-start)

## __Installation__
See [Quick Start](../README.md#quick-start)

## __Flags and Options__
See [Flags](../README.md#command-line-options)

## __Quirks__
Right now the -d. --dump, -r, and --read flags don't need a format. A warning will be printed but can be ignored as the EEPROM dump will still be pretty printed.

## __Updating__
Updates can be performed by just running
```sh
make install
```
and the new version will be installed over the existing verison. 

## __Uninstall__
To uninstall just run 
```sh
make uninstall
```
from your source code directory.

## __Features, Bugs, and Contributing__
If there's anything you'd like to add or find a bug please open an [issue](https://github.com/andrewteall/pi-eeprom-programmer/issues). If you'd like to implement those chages yourself please feel free to open a PR or fork the repository and make it your own.


## __License__
This software project is licensed under the [MIT-0 License](LICENSE). You are free to use, modify, and distribute the project according to the terms of the license.