# ---------------------------------------------------------------------------
# tools/appmake/Makefile — builds the legacy ZX Spectrum app converter.
# Invoked by tools/Makefile or standalone: make (from this directory).
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../..)
DIST_DIR  ?= $(ROOT)/bin/y
HOST_BIN_DIR ?= $(DIST_DIR)/bin

BUILD_DIR := $(ROOT)/build/tools/appmake
BIN_DIR   := $(HOST_BIN_DIR)

CXX       := g++
CXXFLAGS  := -std=c++20 -O2 -Wall -Wextra -Iinclude -I$(ROOT)/include

TARGET    := $(BIN_DIR)/appmake
SRC       := main.cpp util.cpp tape.cpp basic.cpp analysis.cpp app_image.cpp cli.cpp zx_reference.cpp ../microdrive/libmicrodrive.cpp
OBJ       := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all
all: mkdirs $(TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
