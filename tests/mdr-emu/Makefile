ROOT      ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(ROOT)/build/mdr-emu
BIN_DIR   := $(ROOT)/bin/mdr-emu
MDR_BUILD_DIR := $(ROOT)/build/tools/microdrive

CXX      ?= c++
CC       ?= cc
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
CFLAGS   ?= -O2 -Wall -Wextra -pedantic

Z80_DIR := $(THIS_DIR)/vendor/superzazu-z80
MDR_LIB := $(MDR_BUILD_DIR)/libmicrodrive.a

.DEFAULT_GOAL := all

.PHONY: all
all: $(BIN_DIR)/mdr_emu

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

$(MDR_LIB):
	$(MAKE) -C $(ROOT)/tools/microdrive all

$(BIN_DIR)/mdr_emu: $(BUILD_DIR)/mdr_emu.o $(BUILD_DIR)/z80.o $(MDR_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(BUILD_DIR)/mdr_emu.o $(BUILD_DIR)/z80.o $(MDR_LIB)

$(BUILD_DIR)/mdr_emu.o: mdr_emu.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(THIS_DIR) -I$(Z80_DIR) -I$(ROOT)/tools/microdrive -c -o $@ $<

$(BUILD_DIR)/z80.o: $(Z80_DIR)/z80.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(Z80_DIR) -c -o $@ $<

.PHONY: test
test: $(BIN_DIR)/mdr_emu
	$(BIN_DIR)/mdr_emu smoke

.PHONY: stress
stress: $(BIN_DIR)/mdr_emu
	$(BIN_DIR)/mdr_emu stress 5

.PHONY: repro-strict
repro-strict: $(BIN_DIR)/mdr_emu
	$(BIN_DIR)/mdr_emu repro-strict

.PHONY: size
size: $(BIN_DIR)/mdr_emu
	$(BIN_DIR)/mdr_emu size
