all: piepro  

.PHONY: all clean

piepro: piepro.c
		mkdir -p bin
		gcc -o bin/$@ $@.c -l wiringPi

clean:
		rm bin/piepro