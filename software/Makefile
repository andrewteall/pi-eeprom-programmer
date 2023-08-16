

CFLAGS = -Wall -g 

CFLAGS_RELEASE = 

BIN_DIR = bin/
LIB_DIR = lib/

SRC_PATH = src/
INCLUDE_DIRS = -Iinclude/

OUT_FILE = piepro

all: piepro  

.PHONY: all clean gpio

piepro: 
		if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR);fi
		gcc $(CFLAGS) $(INCLUDE_DIRS) -o $(BIN_DIR)$(OUT_FILE) $(SRC_PATH)utils.c $(SRC_PATH)main.c $(SRC_PATH)$@.c  \
			$(SRC_PATH)gpio.c  -lgpiod

gpio_alt:
		gcc -o $(BIN_DIR)$@ $(SRC_PATH)$@.c

clean:
		rm $(BIN_DIR)$(OUT_FILE) 
		rm $(BIN_DIR)gpio