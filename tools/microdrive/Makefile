# ---------------------------------------------------------------------------
# tools/microdrive/Makefile — builds the ZX Spectrum Microdrive image tool.
# Invoked by tools/Makefile or standalone: make (from this directory).
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)

BUILD_DIR :=  $(ROOT)/build/tools/microdrive
BIN_DIR   :=  $(ROOT)/bin/tools/microdrive

CXX       :=  g++
CXXFLAGS  :=  -std=c++20 -O2 -Wall -Wextra

LIB_TARGET    := $(BUILD_DIR)/libmicrodrive.a
CLI_TARGET    := $(BIN_DIR)/microdrive
COMPAT_TARGET := $(BIN_DIR)/mdr

LIB_SRC := libmicrodrive.cpp
CLI_SRC := microdrive.cpp
LIB_OBJ := $(BUILD_DIR)/libmicrodrive.o
CLI_OBJ := $(BUILD_DIR)/microdrive.o

.PHONY: all
all: mkdirs $(LIB_TARGET) $(CLI_TARGET) $(COMPAT_TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(LIB_OBJ): $(LIB_SRC) libmicrodrive.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(CLI_OBJ): $(CLI_SRC) libmicrodrive.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(LIB_TARGET): $(LIB_OBJ)
	ar rcs $@ $^

$(CLI_TARGET): $(CLI_OBJ) $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(CLI_OBJ) $(LIB_TARGET)

$(COMPAT_TARGET): $(CLI_TARGET)
	ln -sf microdrive $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
