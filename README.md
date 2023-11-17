# __A Raspberry Pi GPIO EEPROM Writer and Reader__ 

A way to write Parallel and Serial EEPROMs using the Raspberry Pi GPIO pins.

## __Quick Start__


### __Building the Circuit__
Before reading or writing any EEPROM you have to build the apprpriate circuit to do so.

You'll need a breadboard, jumper wires, and a 40-pin connector and cable to attach to the GPIO pins of the Raspberry Pi and a way to connect the opposite end to the breadboard. I like using one of the many Raspberry Pi GPIO Breakout boards available.

__Serial I2C EEPROM Circuit__
```
            Raspberry Pi GPIO                         I2C DIP8 Serial EEPROM
            3V3  (1) (2)  5V                   GND <-  A0 | (1) (8) | VCC -> 3V3
 SDA 5 -> GPIO2  (3) (4)  5V                   GND <-  A1 | (2) (7) | WP  -> GPIO16 (36)
 SCL 6 -> GPIO3  (5) (6)  GND                  GND <-  A2 | (3) (6) | SCL -> GPIO2  (3)
          GPIO4  (7) (8)  GPIO14               GND <- GND | (4) (5) | SDA -> GPIO3  (5)
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
         GPIO19 (35) (36) GPIO16 -> 7 WP
         GPIO26 (37) (38) GPIO20
            GND (39) (40) GPIO21
   ```

   __Parallel EEPROM Circuit__
```
 IC  |   Raspberry Pi GPIO      | IC  |        Pi  |  DIP28 Parallel EEPROM  |  Pi         | DIP24 Parallel EEPROM
          3V3  (1) (2)  5V            |  (7)  GPIO4 <-  A14  (1) (28) VCC  -> 5V           |
I/O3 -> GPIO2  (3) (4)  5V            | (11) GPIO17 <-  A12  (2) (27) /WE  -> GPIO14 (8)   |
I/O4 -> GPIO3  (5) (6)  GND           | (13) GPIO27 <-   A7  (3) (26) A13  -> GPIO15 (10)  | VCC -> 5V
A14 ->  GPIO4  (7) (8)  GPIO14 -> /WE | (15) GPIO22 <-   A6  (4) (25) A8   -> GPIO18 (12)  | 
          GND  (9) (10) GPIO15 -> A13 | (19) GPIO10 <-   A5  (5) (24) A9   -> GPIO23 (16)  | 
A12 -> GPIO17 (11) (12) GPIO18 -> A8  | (21)  GPIO9 <-   A4  (6) (23) A11  -> GPIO24 (18)  | /WE -> GPIO14 (8)
 A7 -> GPIO27 (13) (14) GND           | (23) GPIO11 <-   A3  (7) (22) /OE  -> GPIO25 (22)  | 
 A6 -> GPIO22 (15) (16) GPIO23 -> A9  | (27)  GPIO0 <-   A2  (8) (21) A10  -> GPIO8  (24)  | 
          3V3 (17) (18) GPIO24 -> A11 | (29)  GPIO5 <-   A1  (9) (20) /CE  -> GPIO7  (26)  | 
 A5 -> GPIO10 (19) (20) GND           | (31)  GPIO6 <-   A0 (10) (19) I/O7 -> GPIO1  (28)  | 
 A4 ->  GPIO9 (21) (22) GPIO25 -> /OE | (33) GPIO13 <- I/O0 (11) (18) I/O6 -> GPIO12 (32)  | 
 A3 -> GPIO11 (23) (24) GPIO8  -> A10 | (35) GPIO19 <- I/O1 (12) (17) I/O5 -> GPIO16 (36)  | 
          GND (25) (26) GPIO7  -> /CE | (37) GPIO26 <- I/O2 (13) (16) I/O4 -> GPIO3  (38)  | 
 A2 ->  GPIO0 (27) (28) GPIO1  -> I/O7|          GND <- GND (14) (15) I/O3 -> GPIO2  (40)  | 
 A1 ->  GPIO5 (29) (30) GND           |
 A0 ->  GPIO6 (31) (32) GPIO12 -> I/O6| 
I/O0 ->GPIO13 (33) (34) GND           |
I/O1 ->GPIO19 (35) (36) GPIO16 -> I/O5| 
I/O2 ->GPIO26 (37) (38) GPIO20        | 
          GND (39) (40) GPIO21 ->     | 
   ```

   Alternatively a pcb can be ordered from any of the various manufacturers from the zipped gerbers contained in `hardware/manufacturing/pi-eeprom-programmer-v.v.v.zip`.

### __Installation and Raspberry Pi Setup__

If you plan to use serial EEPROMs you'll need to setup I2C on the Raspberry Pi. [Adafruit](https://learn.adafruit.com/adafruits-raspberry-pi-lesson-4-gpio-setup/configuring-i2c) has a great tutorial. Typically, if you're just using parallel EEPROM's then you can proceed to the next step. However, if the `libgpiod2` and `libgpiod-dev` packages are not installed on your Raspberry Pi you will need to to install them.

Download and compile the programmer software
```
git clone git@github.com:andrewteall/pi-eeprom-programmer.git
cd pi-eeprom-programmer/software
make
```

Install piepro

```
make install
```

That's it. You should be able to start reading and writing EEPROMs.

### __Installing the IC__
If you've recreated the circuit exactly from above or are using the pcb the IC should be installed so that the bottom left pin of the IC is aligned to the bottom left of the circuit.

### __Reading from an EEPROM__

To read an EEPROM, after building the circuit and inserting the IC, run the command
```sh
piepro -d -m MODEL_TYPE
```
Where MODEL_TYPE is typically the IC's part number such as at28c64, xl2816, at24c02, etc... This will dump the EEPROM's contents to standard out in a pretty format. If you wish to start or limit the addresses dumped you may specify the `-s ADDRESS` or `-l ADDRESS` flags respectively.

### __Writing to an EEPROM__

To write to an EEPROM, after building the circuit and inserting the IC, run the command
```sh
piepro -w file.bin -m MODEL_TYPE
```
To compare you EEPROM to the file after writing, run the command

```sh
piepro -c file.bin -m MODEL_TYPE
```
Where MODEL_TYPE is typically the IC's part number such as at28c64, xl2816, at24c02, etc...

#### __A Quick Note About EEPROM Voltages__
Most modern EERPOMs will work but if you're using an obscure EEPROM you need to makes sure it can operate on 5v Vcc and 3.3v levels if it's a parallel EEPROM and 3.3v Vcc and 3.3v levels if it's a serial EEPROM.

## __Command Line Options__
```
Usage: piepro [options]
Options:
 -c FILE,   --compare FILE  Compare FILE and EEPROM and print number of differences.
            --chipname      Specify the chipname to use. Default: gpiochip0
 -d [N],    --dump [N]      Dump the contents of the EEPROM, 0=PRETTY, 1=BINARY, 2=TEXT, 3=LABELED. Default: PRETTY
 -e [N],    --erase [N]     Erase eeprom with specified byte. Default: 0xFF
 -f,        --force         Force writing of every byte instead of checking for existing value first.
 -id,       --i2c-device-id The address id of the I2C device.
 -h,        --help          Print this message and exit.
 -l N,      --limit N       Specify the maximum address to operate.
 -m MODEL,  --model MODEL   Specify EERPOM device model. Default: AT28C16.
            --no-validate-write 
                            Do not perform a read directly after writing to verify the data was written.
 -r [N],    --read [N]      Read the contents of the EEPROM, 0=PRETTY, 1=BINARY, 2=TEXT, 3=LABELED. Default: PRETTY
 -rb N,     --read-byte ADDRESS 
                            Read From specified ADDRESS.
 -q [N],     --quick [N]    Operates on N bytes at once for reads. Page Size if unspecified or writes. 
                            Implied --force and --no-validate-write.
 -s N,      --start N       Specify the minimum address to operate.
 -t,        --text          Interpret file as a text. Default: binary
                            Text File format:
                            [00000000]00000000 00000000
 -v N,      --v[vvvv]       Set the log verbosity to N, 0=OFF, 1=FATAL, 2=ERROR, 3=WARNING, 4=INFO, 5=DEBUG. Default: WARNING
            --version       Print the piepro version and exit.
 -w FILE,   --write FILE    Write EEPROM with specified file.
 -wb ADDRESS DATA, --write-byte ADDRESS DATA 
                            Write specified DATA to ADDRESS.
 -wd [N],   --write-delay N Enable write delay. N Number of microseconds to delay between writes.


 Supported and tested Models:
        Note that the program may work if your EEPROM uses a similar pinout and
        voltages to the ones listed.
 Model       Size(B)  Addr Len  Data Len   Write Cycle(uS)  Page Size(B) Address Size(B)
----------------------------------------------------------------------------------------
xl2816        2048       11         8          10000            -1            -1
xl28c16       2048       11         8          10000            -1            -1
at28c16       2048       11         8           5000            -1            -1
at28c64       8192       13         8          10000            -1            -1
at28c256     32768       15         8           1000            -1            -1
at24c01        128        7         8           5000             8             1
at24c02        256        8         8           5000             8             1
at24c04        512        9         8           5000            16             2
at24c08       1024       10         8           5000            16             2
at24c16       2048       11         8           5000            16             2
at24c32       4096       12         8           5000            32             2
at24c64       8192       13         8           5000            32             2
at24c128     16384       14         8           5000            64             2
at24c256     32768       15         8           5000            64             2
at24c512     65536       16         8           5000           128             2
 ```

 ## __Features, Bugs, and Contributing__
If there's anything you'd like to add or find a bug please open an [issue](https://github.com/andrewteall/pi-eeprom-programmer/issues). If you'd like to implement those chages yourself please feel free to open a PR or fork the repository and make it your own.


## __License__
This repository is licensed under the [MIT-0 License](LICENSE) for everything not contained in the hardware folder and it's sub folders. For all files and folders in the hardware folder and it's sub folders, they are licensed under [CC0](https://creativecommons.org/publicdomain/zero/1.0/). You are free to use, modify, and distribute the project according to the terms of these licenses.