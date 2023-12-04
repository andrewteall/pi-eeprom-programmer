################################################################################
# Global Variables
################################################################################
# Compiler
CC := gcc
# CC := clang

# Output Targets
TARGET_EXEC := piepro
TARGET_TEST_EXEC := test_harness_piepro
TARGET_LIB := libpiepro.so

# Libraries needed for Linker
LIBRARIES := gpiod

# Folder Paths
BIN_DIR := bin/
LIB_DIR := lib/
MAN_DIR := man/
SRC_DIR := src/
TEST_SRC_DIR := test/
INC_DIR := include/
BUILD_DIR := build/

# Lib Build Config
LIB_EXCLUDE_FILES := main.o
LIB_EXCLUDE_FILES := $(addprefix $(BUILD_DIR)$(SRC_DIR),$(LIB_EXCLUDE_FILES))

# These libs are needed to run(the former) and to build (the latter)
# Not used in current Makefile
DEPS := libgpiod2 libgpiod-dev

################################################################################
# Environment Variables
################################################################################
# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

# MAN_PATH is environment variable, but if it is not set, then set default value
ifeq ($(MAN_PATH),)
    MAN_PATH := /usr/local/man/man1/
endif

################################################################################
# Flags
################################################################################
# Define Include Flags
INC_FLAGS := $(addprefix -I,$(INC_DIR))

# Define Linker Flags
LDFLAGS := $(addprefix -l,$(LIBRARIES))

# Define Compiler Flags
CFLAGS := $(INC_FLAGS) -Wall
CFLAGS_LIB := $(INC_FLAGS) -Wall -shared -fPIC
CFLAGS_TEST := $(INC_FLAGS) -Wall -DWITH_COLOR

################################################################################
# Source and Object Variables
################################################################################
# Define all the C files we want to compile
SRCS := $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c' -printf '%f\n')
TEST_SRCS := $(shell find $(TEST_SRC_DIR) -maxdepth 1 -name '*.c' -printf '%f\n')

# Define Output objects from Src Groups. 
# Convert *.c names to *.o and prepend the build path
OBJS := $(subst .c.o,.o,$(SRCS:%=$(BUILD_DIR)$(SRC_DIR)%.o))
LIB_OBJS := $(subst .c.o,.o,$(SRCS:%=$(BUILD_DIR)$(LIB_DIR)%.o))
TEST_OBJS := $(subst .c.o,.o,$(TEST_SRCS:%=$(BUILD_DIR)$(TEST_SRC_DIR)%.o))

################################################################################
# Object Build
################################################################################
# Object step for C source bin
$(BUILD_DIR)$(SRC_DIR)%.o: $(SRC_DIR)%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $^ -o $@

# Object step for C source lib
$(BUILD_DIR)$(LIB_DIR)%.o: $(SRC_DIR)%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_LIB) -c $^ -o $@

# Object step for C source test
$(BUILD_DIR)$(TEST_SRC_DIR)%.o: $(TEST_SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_TEST) -c $^ -o $@

################################################################################
# Target Build
################################################################################
.PHONY: all
all: piepro test_harness_piepro libpiepro

.PHONY: piepro
piepro: $(BIN_DIR)$(TARGET_EXEC)

.PHONY: libpiepro
libpiepro: $(LIB_DIR)$(TARGET_LIB)

.PHONY: test_harness_piepro
test_harness_piepro: $(BIN_DIR)$(TARGET_TEST_EXEC)

# Bin Build
$(BIN_DIR)$(TARGET_EXEC): $(OBJS)
		mkdir -p $(BIN_DIR)
		$(CC) -o $(BIN_DIR)$(TARGET_EXEC) $(OBJS) $(LDFLAGS)

# Lib Build
$(LIB_DIR)$(TARGET_LIB): $(LIB_OBJS)
		mkdir -p $(LIB_DIR)
		$(CC) -o $(LIB_DIR)$(TARGET_LIB) $(filter-out $(LIB_EXCLUDE_FILES),$(LIB_OBJS)) $(LDFLAGS)

# Test Build
$(BIN_DIR)$(TARGET_TEST_EXEC): $(TEST_OBJS) $(OBJS)
		mkdir -p $(BIN_DIR)
		$(CC) -o $(BIN_DIR)$(TARGET_TEST_EXEC) $(TEST_OBJS) $(filter-out $(LIB_EXCLUDE_FILES),$(OBJS)) $(LDFLAGS)

################################################################################
# Test
################################################################################
.PHONY: test
test: test_harness_piepro test_run

.PHONY: test_I2C
test_I2C: test_harness_piepro test_run_I2C

.PHONY: test_run
test_run:
		$(BIN_DIR)$(TARGET_TEST_EXEC)

.PHONY: test_run_I2C
test_run_I2C:
		$(BIN_DIR)$(TARGET_TEST_EXEC) 1

################################################################################
# Install and Clean
################################################################################
.PHONY: install-all
install-all: install install-lib

.PHONY: uninstall-all
uninstall-all: uninstall uninstall-lib

.PHONY: install
install:
		mkdir -p $(DESTDIR)$(PREFIX)/$(BIN_DIR)
		install $(BIN_DIR)$(TARGET_EXEC) $(DESTDIR)$(PREFIX)/$(BIN_DIR)$(TARGET_EXEC)
		mkdir -p $(MAN_PATH)
		install -g 0 -o 0 -m 0644 $(MAN_DIR)$(TARGET_EXEC).1 $(MAN_PATH)
		gzip $(MAN_PATH)$(TARGET_EXEC).1

.PHONY: install-lib
install-lib:
		mkdir -p $(DESTDIR)$(PREFIX)/$(LIB_DIR)
		install $(LIB_DIR)$(TARGET_LIB) $(DESTDIR)$(PREFIX)/$(LIB_DIR)$(TARGET_LIB)
		mkdir -p $(DESTDIR)$(PREFIX)/$(INC_DIR)
		install $(INC_DIR)$(TARGET_EXEC).h $(DESTDIR)$(PREFIX)/$(INC_DIR)$(TARGET_EXEC).h
		ldconfig

.PHONY: uninstall
uninstall:
		if [ -f $(DESTDIR)$(PREFIX)/$(BIN_DIR)$(TARGET_EXEC) ]; then rm $(DESTDIR)$(PREFIX)/$(BIN_DIR)$(TARGET_EXEC) ;fi
		if [ -f $(MAN_PATH)$(TARGET_EXEC).1.gz ]; then rm $(MAN_PATH)$(TARGET_EXEC).1.gz ;fi

.PHONY: uninstall-lib
uninstall-lib:
		if [ -f $(DESTDIR)$(PREFIX)/$(LIB_DIR)$(TARGET_LIB) ]; then rm $(DESTDIR)$(PREFIX)/$(LIB_DIR)$(TARGET_LIB) ;fi
		if [ -f $(DESTDIR)$(PREFIX)/$(INC_DIR)$(TARGET_EXEC).h ]; then rm $(DESTDIR)$(PREFIX)/$(INC_DIR)$(TARGET_EXEC).h ;fi
		ldconfig

.PHONY: clean
clean:
		if [ -f $(BIN_DIR)$(TARGET_EXEC) ]; then rm $(BIN_DIR)$(TARGET_EXEC);fi
		if [ -f $(LIB_DIR)$(TARGET_LIB) ]; then rm $(LIB_DIR)$(TARGET_LIB);fi
		if [ -f $(BIN_DIR)$(TARGET_TEST_EXEC) ]; then rm $(BIN_DIR)$(TARGET_TEST_EXEC);fi
		if [ -d $(BUILD_DIR) ]; then rm -r $(BUILD_DIR);fi

.PHONY: test_clean
test_clean:
		if [ -f $(BIN_DIR)$(TARGET_TEST_EXEC) ]; then rm $(BIN_DIR)$(TARGET_TEST_EXEC);fi
