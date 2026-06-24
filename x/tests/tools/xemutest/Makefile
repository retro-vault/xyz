_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
X_ROOT ?= $(abspath $(_THIS_DIR)/../../..)
REPO_ROOT ?= $(abspath $(X_ROOT)/..)
BUILD_DIR ?= $(REPO_ROOT)/build
DIST_DIR ?= $(REPO_ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib

CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -g

XEMU_DIR := $(X_ROOT)/lib/xemu
XEMU_INCLUDE := -I$(XEMU_DIR)/include
XEMU_LIB := $(PUBLIC_LIB_DIR)/libxemu.a
XEMU_SRCS := $(wildcard $(XEMU_DIR)/src/*.cpp)
XEMU_HDRS := $(wildcard $(XEMU_DIR)/include/xemu/*.h)

XBFD_DIR := $(X_ROOT)/lib/xbfd
XBFD_INCLUDE := -I$(XBFD_DIR)/include
XBFD_LIB := $(BUILD_DIR)/lib/libxbfd.a

XZ80_DIR := $(X_ROOT)/lib/xz80
XZ80_INCLUDE := -I$(XZ80_DIR)/include
XZ80_LIB := $(PUBLIC_LIB_DIR)/libxz80.a

RSP_DIR := $(X_ROOT)/lib/rsp
RSP_INCLUDE := -I$(RSP_DIR)/include
RSP_LIB := $(PUBLIC_LIB_DIR)/librsp.a

OUTPUT_DIR := $(BUILD_DIR)/tests/tools/xemutest
TARGET := $(OUTPUT_DIR)/xemutest
SOURCES := $(wildcard src/*.cpp)
SUITE_ROOT := $(X_ROOT)/tests/tests
RUNNER := $(X_ROOT)/tests/run_tests.sh

.PHONY: all test clean stage-toolchain

all: $(TARGET)

$(TARGET): $(SOURCES) $(XEMU_LIB) $(XZ80_LIB) $(RSP_LIB) $(XBFD_LIB)
	mkdir -p $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) $(XEMU_INCLUDE) $(XZ80_INCLUDE) $(RSP_INCLUDE) $(XBFD_INCLUDE) \
		$(SOURCES) $(XEMU_LIB) $(RSP_LIB) $(XZ80_LIB) $(XBFD_LIB) -pthread -o $@

$(XEMU_LIB): $(XEMU_SRCS) $(XEMU_HDRS)
	$(MAKE) -C $(X_ROOT)/lib/xemu \
		ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(XZ80_LIB):
	$(MAKE) -C $(XZ80_DIR) \
		ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(RSP_LIB):
	$(MAKE) -C $(RSP_DIR) \
		ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR) \
		ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

stage-toolchain:
	$(MAKE) -C $(X_ROOT)/src \
		ROOT=$(X_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		DEFAULT_PLATFORM=emu
	$(MAKE) -C $(X_ROOT) \
		REPO_ROOT=$(REPO_ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) \
		HOST_BIN_DIR=$(HOST_BIN_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR) \
		PLATFORM=emu DEFAULT_PLATFORM=emu \
		stage-includes stage-xcc-support

test: all stage-toolchain
	XCC=$(HOST_BIN_DIR)/xcc bash $(RUNNER) --work $(OUTPUT_DIR)/work

clean:
	rm -rf $(OUTPUT_DIR)
