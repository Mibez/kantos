# @file Makefile
#
# Copyright (c) 2025 Miikka Lukumies

# Path to toolchain
CROSS_COMPILE = ./toolchain/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-

# Executables used to compile, link, and format the executable
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
GDB = $(CROSS_COMPILE)gdb
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf

INCLUDES := -Idrivers/ -Ilibs/ -Ios/


# Optional tuning flags for compiler
# TUNE_CFLAGS =  -Wall -Werror
TUNE_CFLAGS = -ffunction-sections -fdata-sections -Wall -Werror

# Mandatory flags for compiling
CFLAGS = $(INCLUDES) -fno-exceptions -mcpu=cortex-m33 -mthumb -g -nostdlib -nostartfiles -fno-builtin -ffreestanding $(TUNE_CFLAGS) 

# Linker flags
# LINKER_FLAGS = 
LINKER_FLAGS = --gc-sections

# Executable used to figure out route to Windows host from WSL
HOSTNAME=`hostname`

# Directory definitions
BOOT_DIR := arch/arm/cortex-m33
OS_DIR := os
LIBS_DIR := libs
BUILD_DIR := build

# Define source files
BOOT_SRC := $(BOOT_DIR)/boot_cortex_m33.s
BOARD_SRCS := $(wildcard $(BOOT_DIR)/drivers/*/*.c)
LIB_SRCS := $(wildcard $(LIBS_DIR)/*/*.c)
MAIN_SRC := $(OS_DIR)/app.c
OS_SRCS := $(OS_DIR)/os.c

# Define object files
BOOT_OBJ := $(BUILD_DIR)/boot_cortex_m33.o
BOARD_OBJS := $(patsubst $(BOOT_DIR)/drivers/%.c, $(BUILD_DIR)/drivers/%.o, $(BOARD_SRCS))
LIBS_OBJS := $(patsubst $(LIBS_DIR)/%.c, $(BUILD_DIR)/libs/%.o, $(LIB_SRCS))
MAIN_OBJ := $(BUILD_DIR)/app.o
OS_OBJ := $(BUILD_DIR)/os.o

# Define linker script
LINKER_SCRIPT := $(BOOT_DIR)/link_cortex_m33.ld

# Define output executable name
TARGET := $(BUILD_DIR)/kernel.elf

.PHONY: all clean

all: $(TARGET)

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/libs
	mkdir -p $(BUILD_DIR)/libs/print
	mkdir -p $(BUILD_DIR)/drivers
	mkdir -p $(BUILD_DIR)/drivers/led
	mkdir -p $(BUILD_DIR)/drivers/system
	mkdir -p $(BUILD_DIR)/drivers/uart

# Rule to build the final executable
$(TARGET): $(BOOT_OBJ) $(MAIN_OBJ) $(OS_OBJ) $(BOARD_OBJS) $(LIBS_OBJS) $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	$(LD) -T $(LINKER_SCRIPT) $(LINKER_FLAGS) $(BOOT_OBJ) $(MAIN_OBJ) $(OS_OBJ) $(BOARD_OBJS) $(LIBS_OBJS) -o $@
	$(OBJDUMP) -D $(TARGET) > $(BUILD_DIR)/kernel.list

# Rule to compile boot.s into boot.o
$(BOOT_OBJ): $(BOOT_SRC) $(BUILD_DIR)
	@echo "Assembling $< to $@"
	$(CC) $(CFLAGS) -c $< -o $(BOOT_OBJ)

# Rule to compile main.c into main.o
$(MAIN_OBJ): $(MAIN_SRC) $(BUILD_DIR)
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule to compile os.c into os.o
$(OS_OBJ): $(OS_SRCS) $(BUILD_DIR)
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule to compile arch drivers
$(BUILD_DIR)/drivers/%.o: $(BOOT_DIR)/drivers/%.c 
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile libs
$(BUILD_DIR)/libs/%.o: $(LIBS_DIR)/%.c 
	@echo "Compiling $< to $@"
	$(CC) $(CFLAGS) -c $< -o $@


# Clean rule
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Rule for running GDB on WSL targeting windows host
gdbw: $(TARGET)
	$(GDB) $^ -ex "target remote $(HOSTNAME).local:61234" --se=$(TARGET)
