
CC = gcc
CFLAGS = -Wall
CFLAGS_DEV = -Wall -g 

BIN_DIR = ./bin
LIB_DIR = ./lib

SRC_PATH = src
INCLUDE_DIRS = -Iinclude/
OUT_FILE = piepro

DEPS = libgpiod2 libgpiod-dev

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

all: piepro  

.PHONY: all clean install uninstall

piepro: 
		if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR);fi
		$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(BIN_DIR)/$(OUT_FILE) $(SRC_PATH)/utils.c $(SRC_PATH)/main.c $(SRC_PATH)/$@.c \
			$(SRC_PATH)/gpio.c -lgpiod

clean:
		if [ -f $(BIN_DIR)/$(OUT_FILE) ]; then rm $(BIN_DIR)/$(OUT_FILE);fi

install:
		if [ ! -d $(DESTDIR)$(PREFIX)/bin ]; then mkdir -p $(DESTDIR)$(PREFIX)/bin;fi
		install $(BIN_DIR)/$(OUT_FILE) $(DESTDIR)$(PREFIX)/bin/$(OUT_FILE)

uninstall:
		if [ -f $(DESTDIR)$(PREFIX)/bin/$(OUT_FILE) ]; then rm $(DESTDIR)$(PREFIX)/bin/$(OUT_FILE);fi