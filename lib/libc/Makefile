_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT ?= $(abspath $(_THIS_DIR)/../..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
TARGET_DIR ?= $(DIST_DIR)/z80
TARGET_LIB_DIR ?= $(TARGET_DIR)/lib
TARGET_INCLUDE_DIR ?= $(TARGET_DIR)/include

BUILD_TARGET := $(BUILD_DIR)/lib/libc.a
DIST_TARGET  := $(TARGET_LIB_DIR)/libc.a

.PHONY: all build clean stage-headers

all: $(DIST_TARGET) stage-headers

build:
	$(MAKE) -C src ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(HOST_BIN_DIR) WITH_SYS=0 AR8FLAGS="--mode=gnu rcs"

$(DIST_TARGET): build | $(TARGET_LIB_DIR)
	cp $(BUILD_TARGET) $@

stage-headers: | $(TARGET_INCLUDE_DIR)
	cp -R include/. $(TARGET_INCLUDE_DIR)/

$(TARGET_INCLUDE_DIR):
	mkdir -p $@

$(TARGET_LIB_DIR):
	mkdir -p $@

clean:
	$(MAKE) -C src clean ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(HOST_BIN_DIR)
	rm -f $(DIST_TARGET)
