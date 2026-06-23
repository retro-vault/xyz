# ---------------------------------------------------------------------------
# y/pkg/appmake/Makefile — builds the legacy ZX Spectrum app converter.
# Invoked by the top-level compatibility wrapper or standalone from here.
# ---------------------------------------------------------------------------

_THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
Y_ROOT     ?= $(abspath $(_THIS_DIR)/../..)
ROOT       ?= $(abspath $(_THIS_DIR)/../../..)
DIST_DIR   ?= $(ROOT)/bin/y
HOST_BIN_DIR ?= $(DIST_DIR)/bin

BUILD_ROOT := $(if $(BUILD_DIR),$(BUILD_DIR),$(ROOT)/build)
APPMAKE_BUILD_DIR := $(BUILD_ROOT)/y/pkg/appmake
BIN_DIR   := $(HOST_BIN_DIR)

CXX       := g++
CXXFLAGS  := -std=c++20 -O2 -Wall -Wextra -Iinclude -I$(Y_ROOT)/include

TARGET    := $(BIN_DIR)/appmake
APPMAKE_SRC := main.cpp util.cpp tape.cpp basic.cpp analysis.cpp app_image.cpp cli.cpp zx_reference.cpp
MICRODRIVE_SRC := ../microdrive/libmicrodrive.cpp
OBJ       := $(APPMAKE_SRC:%.cpp=$(APPMAKE_BUILD_DIR)/%.o) $(APPMAKE_BUILD_DIR)/microdrive/libmicrodrive.o

.PHONY: all
all: mkdirs $(TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(APPMAKE_BUILD_DIR) $(BIN_DIR)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(APPMAKE_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(APPMAKE_BUILD_DIR)/microdrive/%.o: ../microdrive/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(APPMAKE_BUILD_DIR) $(BIN_DIR)
