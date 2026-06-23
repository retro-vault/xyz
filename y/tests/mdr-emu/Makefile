REPO_ROOT ?= $(realpath ../../..)
Y_ROOT    ?= $(realpath ../..)
THIS_DIR  := $(realpath .)
BUILD_DIR := $(REPO_ROOT)/build/mdr-emu
DIST_DIR  ?= $(REPO_ROOT)/bin/x
TOOL_DIST_DIR ?= $(REPO_ROOT)/bin/y
PUBLIC_LIB_DIR ?= $(TOOL_DIST_DIR)/lib
BIN_DIR   := $(BUILD_DIR)

CXX      ?= c++
CC       ?= cc
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic
CFLAGS   ?= -O2 -Wall -Wextra -pedantic

Z80_DIR := $(THIS_DIR)/vendor/superzazu-z80
MDR_LIB := $(PUBLIC_LIB_DIR)/libmicrodrive.a
YOS_ROM := $(REPO_ROOT)/bin/y/z80/spectrum/bin/yos.rom
YOS_CDB := $(REPO_ROOT)/bin/y/z80/spectrum/bin/yos.cdb

.DEFAULT_GOAL := all

.PHONY: all
all: $(BIN_DIR)/mdr_emu

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

$(MDR_LIB):
	$(MAKE) -C $(Y_ROOT)/pkg/microdrive all ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT)

$(YOS_ROM) $(YOS_CDB):
	$(MAKE) -C $(Y_ROOT)/src ROOT=$(REPO_ROOT) REPO_ROOT=$(REPO_ROOT) Y_ROOT=$(Y_ROOT) X_ROOT=$(REPO_ROOT)/x BUILD_DIR=$(REPO_ROOT)/build/yos DIST_DIR=$(REPO_ROOT)/bin/y ZX_ROMS_DIR=$(REPO_ROOT)/bin/y/z80/spectrum/bin

$(BIN_DIR)/mdr_emu: $(BUILD_DIR)/mdr_emu.o $(BUILD_DIR)/z80.o $(MDR_LIB)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(BUILD_DIR)/mdr_emu.o $(BUILD_DIR)/z80.o $(MDR_LIB)

$(BUILD_DIR)/mdr_emu.o: mdr_emu.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(THIS_DIR) -I$(Z80_DIR) -I$(Y_ROOT)/include -c -o $@ $<

$(BUILD_DIR)/z80.o: $(Z80_DIR)/z80.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(Z80_DIR) -c -o $@ $<

.PHONY: test
test: $(BIN_DIR)/mdr_emu $(YOS_ROM) $(YOS_CDB)
	$(BIN_DIR)/mdr_emu smoke

.PHONY: stress
stress: $(BIN_DIR)/mdr_emu $(YOS_ROM) $(YOS_CDB)
	$(BIN_DIR)/mdr_emu stress 5

.PHONY: repro-strict
repro-strict: $(BIN_DIR)/mdr_emu $(YOS_ROM) $(YOS_CDB)
	$(BIN_DIR)/mdr_emu repro-strict

.PHONY: size
size: $(BIN_DIR)/mdr_emu $(YOS_ROM) $(YOS_CDB)
	$(BIN_DIR)/mdr_emu size
