_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT ?= $(abspath $(_THIS_DIR)/..)
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
TARGET_DIR ?= $(DIST_DIR)/z80
TARGET_LIB_DIR ?= $(TARGET_DIR)/lib
TARGET_INCLUDE_DIR ?= $(TARGET_DIR)/include
C8FLAGS ?= -I$(ROOT)/libc/include -I$(ROOT)/platforms/include
AS8FLAGS ?= --mode=sdcc
LIBC_PROFILE ?= full
ifeq ($(LIBC_PROFILE),tiny)
override LIBC_FLOAT     := 0
override LIBC_DOUBLE    := 0
override LIBC_LONG      := 0
override LIBC_LONGLONG  := 0
else
LIBC_FLOAT     ?= 1
LIBC_DOUBLE    ?= 1
LIBC_LONG      ?= 1
LIBC_LONGLONG  ?= 1
endif

LIBC_BUILD_FLAVOR := $(LIBC_PROFILE)-f$(LIBC_FLOAT)-d$(LIBC_DOUBLE)-l$(LIBC_LONG)-ll$(LIBC_LONGLONG)
BUILD_TARGET := $(BUILD_DIR)/lib/libc/$(LIBC_BUILD_FLAVOR)/libc.a
DIST_TARGET  := $(TARGET_LIB_DIR)/libc.a

.PHONY: all build clean stage-headers

all: $(DIST_TARGET) stage-headers

build:
	$(MAKE) -C src ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) HOST_BIN_DIR=$(HOST_BIN_DIR) WITH_SYS=0 AR8FLAGS="--mode=gnu rcs" C8FLAGS="$(C8FLAGS)" AS8FLAGS="$(AS8FLAGS)" LIBC_PROFILE="$(LIBC_PROFILE)" LIBC_FLOAT="$(LIBC_FLOAT)" LIBC_DOUBLE="$(LIBC_DOUBLE)" LIBC_LONG="$(LIBC_LONG)" LIBC_LONGLONG="$(LIBC_LONGLONG)"

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
