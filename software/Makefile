
CC = gcc
CFLAGS = -Wall
CFLAGS_LIB = -Wall -shared -fPIC  

BIN_DIR = bin
LIB_DIR = lib
MAN_DIR = man
SRC_DIR = src
INC_DIR = include

OUT_FILE = piepro
LIB_OUT_FILE = libpiepro.so

INCLUDE_PATH = -I$(INC_DIR)/
MAN_PATH = /usr/local/man

DEPS = libgpiod2 libgpiod-dev

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

.PHONY: all clean install uninstall libpiepro install-lib uninstall-lib install-all uninstall-all

all: piepro

build-all: piepro libpiepro

piepro: 
		if [ ! -d $(BIN_DIR) ]; then mkdir -p $(BIN_DIR);fi
		$(CC) $(CFLAGS) $(INCLUDE_PATH) -o $(BIN_DIR)/$(OUT_FILE) $(SRC_DIR)/utils.c $(SRC_DIR)/main.c $(SRC_DIR)/$@.c \
			$(SRC_DIR)/gpio.c $(SRC_DIR)/ulog.c -lgpiod

libpiepro: 
		if [ ! -d $(LIB_DIR) ]; then mkdir -p $(LIB_DIR);fi
		$(CC) $(CFLAGS_LIB) $(INCLUDE_PATH) -o $(LIB_DIR)/$(LIB_OUT_FILE) $(SRC_DIR)/utils.c $(SRC_DIR)/piepro.c \
			$(SRC_DIR)/gpio.c $(SRC_DIR)/ulog.c -lgpiod

clean:
		if [ -f $(BIN_DIR)/$(OUT_FILE) ]; then rm $(BIN_DIR)/$(OUT_FILE);fi
		if [ -f $(LIB_DIR)/$(LIB_OUT_FILE) ]; then rm $(LIB_DIR)/$(LIB_OUT_FILE);fi

install-all: install install-lib

uninstall-all: uninstall uninstall-lib

install:
		if [ ! -d $(DESTDIR)$(PREFIX)/$(BIN_DIR) ]; then mkdir -p $(DESTDIR)$(PREFIX)/$(BIN_DIR);fi
		install $(BIN_DIR)/$(OUT_FILE) $(DESTDIR)$(PREFIX)/$(BIN_DIR)/$(OUT_FILE)
		install -g 0 -o 0 -m 0644 $(MAN_DIR)/$(OUT_FILE).1 $(MAN_PATH)/man1/
		gzip $(MAN_PATH)/man1/$(OUT_FILE).1

install-lib:
		if [ ! -d $(DESTDIR)$(PREFIX)/lib ]; then mkdir -p $(DESTDIR)$(PREFIX)/lib;fi
		install $(LIB_DIR)/$(LIB_OUT_FILE) $(DESTDIR)$(PREFIX)/$(LIB_DIR)/$(LIB_OUT_FILE)
		if [ ! -d $(DESTDIR)$(PREFIX)/$(INC_DIR) ]; then mkdir -p $(DESTDIR)$(PREFIX)/$(INC_DIR);fi
		install $(INC_DIR)/piepro.h $(DESTDIR)$(PREFIX)/$(INC_DIR)/piepro.h
		ldconfig

uninstall:
		if [ -f $(DESTDIR)$(PREFIX)/$(BIN_DIR)/$(OUT_FILE) ]; then rm $(DESTDIR)$(PREFIX)/$(BIN_DIR)/$(OUT_FILE) ;fi
		if [ -f $(MAN_PATH)/man1/$(OUT_FILE).1.gz ]; then rm $(MAN_PATH)/man1/$(OUT_FILE).1.gz ;fi

uninstall-lib:
		if [ -f $(DESTDIR)$(PREFIX)/$(LIB_DIR)/$(LIB_OUT_FILE) ]; then rm $(DESTDIR)$(PREFIX)/$(LIB_DIR)/$(LIB_OUT_FILE) ;fi
		if [ -f $(DESTDIR)$(PREFIX)/$(INC_DIR)/piepro.h ]; then rm $(DESTDIR)$(PREFIX)/$(INC_DIR)/piepro.h ;fi
		ldconfig