# __A Raspberry Pi GPIO EEPROM Writer and Reader__ 

A way to write Parallel and Serial EEPROMs using the Raspberry Pi GPIO pins.

## __Quick Start__
------

### __Building the Circuit__
Before reading or writing any EEPROM you have to build the apprpriate circuit to do so.

You'll need a breadboard, jumper wires, and a 40-pin connector and cable to attach to the GPIO pins of the Raspberry Pi and a way to connect the opposite end to the breadboard. I like using one of the many Raspberry Pi GPIO Breakout boards available.

__Serial I2C EEPROM Circuit__
```c#
            Raspberry Pi GPIO                         I2C DIP8 Serial EEPROM
            3V3  (1) (2)  5V                   GND <-  A0 | (1) (8) | VCC -> 3V3
 SDA 5 -> GPIO2  (3) (4)  5V                   GND <-  A1 | (2) (7) | WP  -> GND
 SCL 6 -> GPIO3  (5) (6)  GND                  GND <-  A2 | (3) (6) | SCL -> GPIO2 (3)
          GPIO4  (7) (8)  GPIO14               GND <- GND | (4) (5) | SDA -> GPIO3 (5)
            GND  (9) (10) GPIO15
         GPIO17 (11) (12) GPIO18
         GPIO27 (13) (14) GND   
         GPIO22 (15) (16) GPIO23
            3V3 (17) (18) GPIO24
         GPIO10 (19) (20) GND   
          GPIO9 (21) (22) GPIO25
         GPIO11 (23) (24) GPIO8 
            GND (25) (26) GPIO7 
          GPIO0 (27) (28) GPIO1 
          GPIO5 (29) (30) GND   
          GPIO6 (31) (32) GPIO12
         GPIO13 (33) (34) GND   
         GPIO19 (35) (36) GPIO16
         GPIO26 (37) (38) GPIO20
            GND (39) (40) GPIO21
   ```

   __Parallel EEPROM Circuit__
```c#
 IC  |   Raspberry Pi GPIO      | IC  |        Pi  |  DIP28 Parallel EEPROM  |  Pi         | DIP24 Parallel EEPROM
          3V3  (1) (2)  5V            |  (7)  GPIO4 <-  A14  (1) (28) VCC  -> 5V           |
        GPIO2  (3) (4)  5V            | (11) GPIO17 <-  A12  (2) (27) /WE  -> GPIO14 (8)   |
        GPIO3  (5) (6)  GND           | (13) GPIO27 <-   A7  (3) (26) A13  -> GPIO15 (10)  | VCC -> 5V
A14 ->  GPIO4  (7) (8)  GPIO14 -> /WE | (15) GPIO22 <-   A6  (4) (25) A8   -> GPIO18 (12)  | 
          GND  (9) (10) GPIO15 -> A13 | (19) GPIO10 <-   A5  (5) (24) A9   -> GPIO23 (16)  | 
A12 -> GPIO17 (11) (12) GPIO18 -> A8  | (21)  GPIO9 <-   A4  (6) (23) A11  -> GPIO24 (18)  | /WE -> GPIO14 (8)
 A7 -> GPIO27 (13) (14) GND           | (23) GPIO11 <-   A3  (7) (22) /OE  -> GPIO25 (22)  | 
 A6 -> GPIO22 (15) (16) GPIO23 -> A9  | (27)  GPIO0 <-   A2  (8) (21) A10  -> GPIO8  (24)  | 
          3V3 (17) (18) GPIO24 -> A11 | (29)  GPIO5 <-   A1  (9) (20) /CE  -> GPIO7  (26)  | 
 A5 -> GPIO10 (19) (20) GND           | (31)  GPIO6 <-   A0 (10) (19) I/O7 -> GPIO1  (28)  | 
 A4 ->  GPIO9 (21) (22) GPIO25 -> /OE | (33) GPIO13 <- I/O0 (11) (18) I/O6 -> GPIO12 (32)  | 
 A3 -> GPIO11 (23) (24) GPIO8  -> A10 | (35) GPIO19 <- I/O1 (12) (17) I/O5 -> GPIO16 (36)  | 
          GND (25) (26) GPIO7  -> /CE | (37) GPIO26 <- I/O2 (13) (16) I/O4 -> GPIO20 (38)  | 
 A2 ->  GPIO0 (27) (28) GPIO1  -> I/O7|          GND <- GND (14) (15) I/O3 -> GPIO21 (40)  | 
 A1 ->  GPIO5 (29) (30) GND           |
 A0 ->  GPIO6 (31) (32) GPIO12 -> I/O6| 
I/O0 ->GPIO13 (33) (34) GND           |
I/O1 ->GPIO19 (35) (36) GPIO16 -> I/O5| 
I/O2 ->GPIO26 (37) (38) GPIO20 -> I/O4| 
          GND (39) (40) GPIO21 -> I/O3| 
   ```

### __Installation and Raspberry Pi Setup__

If you plan to use serial EEPROMs you'll need to setup I2C on the Raspberry Pi. [SparkFun](https://learn.sparkfun.com/tutorials/raspberry-pi-spi-and-i2c-tutorial/all) has a great tutorial. Typically, if you're just using parallel EEPROM's then you can proceed to the next step. However, if the [WiringPi](https://github.com/WiringPi/WiringPi) library is not installed on your Raspberry Pi you will need to to install it. The SparkFun link above will also descibe those steps.

Download and compile the programmer software
```
git clone git@github.com:andrewteall/pi-eeprom-programmer.git
cd pi-eeprom-programmer
make
cd bin
chmod +x piepro
```

That's it. You should be able to start reading and writing EEPROMs.

### __Reading from an EEPROM__

To read an EEPROM, after building the circuit and inserting the IC, run the command
```sh
./piepro -d 3 -m MODEL_TYPE
```
Where MODEL_TYPE is typically the IC's part number such as at28c64, xl2816, at24c02, etc...

### __Writing to an EEPROM__

To write to EEPROM, after building the circuit and inserting the IC, run the command
```sh
./piepro -m MODEL_TYPE -b file.bin
```
Where MODEL_TYPE is typically the IC's part number such as at28c64, xl2816, at24c02, etc...

## __Command Line Options__
------
```
Usage: piepro [options] [file]
Options:
 -b,   --binary                 Interpret file as a binary. Default: text
                                        Text File format:
                                        00000000 00000000
 -c,    --compare               Compare file and EEPROM and print differences.
 -d N,  --dump N                Dump the contents of the EEPROM, 0=DEFAULT, 1=BINARY, 2=TEXT, 3=PRETTY.
 -f,    --force                 Force writing of every byte instead of checking for existing value first.
 -id,   --i2c-device-id         The id of the I2C device.
 -h,    --help                  Print this message and exit.
 -l N,  --limit N               Specify the maximum address to operate.
        --no-validate-write     Do not perform a read directly after writing to verify the data was written.
 -m MODEL, --model MODEL        Specify EERPOM device model. Default: AT28C16.
 -s N,  --start N               Specify the minimum address to operate.
 -v N,  --v[vvvv]               Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG.
 -w ADDRESS DATA, 
        --write ADDRESS DATA    Write specified DATA to ADDRESS.
 -wd N, --write-delay N         Number of microseconds to delay between writes.
 ```