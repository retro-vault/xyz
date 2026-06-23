# ---------------------------------------------------------------------------
# y/pkg/serial/Makefile — builds the serial ZX Spectrum transfer tool.
# Invoked by the top-level compatibility wrapper or standalone from here.
# ---------------------------------------------------------------------------

_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT       ?= $(abspath $(_THIS_DIR)/../../..)
DIST_DIR   ?= $(ROOT)/bin/y
HOST_BIN_DIR ?= $(DIST_DIR)/bin

BUILD_ROOT := $(if $(BUILD_DIR),$(BUILD_DIR),$(ROOT)/build)
SERIAL_BUILD_DIR := $(BUILD_ROOT)/y/pkg/serial
BIN_DIR   :=  $(HOST_BIN_DIR)

CXX       :=  g++
CXXFLAGS  :=  -std=c++20 -O2 -Wall -Wextra

TARGET    :=  $(BIN_DIR)/serial
SRC       :=  serial.cpp

.PHONY: all
all: mkdirs $(TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(SERIAL_BUILD_DIR) $(BIN_DIR)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(SERIAL_BUILD_DIR) $(BIN_DIR)
