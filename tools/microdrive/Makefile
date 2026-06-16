# ---------------------------------------------------------------------------
# tools/microdrive/Makefile — builds the ZX Spectrum Microdrive image tool.
# Invoked by tools/Makefile or standalone: make (from this directory).
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
DIST_DIR  ?= $(ROOT)/bin/y
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
PUBLIC_INCLUDE_DIR ?= $(DIST_DIR)/include

BUILD_DIR :=  $(ROOT)/build/tools/microdrive
BIN_DIR   :=  $(HOST_BIN_DIR)
LIB_DIR   :=  $(PUBLIC_LIB_DIR)
INCLUDE_DIR := $(PUBLIC_INCLUDE_DIR)

CXX       :=  g++
CXXFLAGS  :=  -std=c++20 -O2 -Wall -Wextra -I$(ROOT)/include

LIB_TARGET    := $(LIB_DIR)/libmicrodrive.a
CLI_TARGET    := $(BIN_DIR)/microdrive

LIB_SRC := libmicrodrive.cpp
CLI_SRC := microdrive.cpp
LIB_OBJ := $(BUILD_DIR)/libmicrodrive.o
CLI_OBJ := $(BUILD_DIR)/microdrive.o

HDR_TARGET    := $(INCLUDE_DIR)/microdrive/microdrive.h

.PHONY: all
all: mkdirs $(LIB_TARGET) $(CLI_TARGET) $(HDR_TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR) $(INCLUDE_DIR)/microdrive

$(HDR_TARGET): $(ROOT)/include/microdrive/microdrive.h
	cp $< $@

$(LIB_OBJ): $(LIB_SRC) $(ROOT)/include/microdrive/microdrive.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(CLI_OBJ): $(CLI_SRC) $(ROOT)/include/microdrive/microdrive.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB_TARGET): $(LIB_OBJ)
	ar rcs $@ $^

$(CLI_TARGET): $(CLI_OBJ) $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(CLI_OBJ) $(LIB_TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR) $(INCLUDE_DIR)/microdrive
