# ---------------------------------------------------------------------------
# tools/serial/Makefile — builds the serial ZX Spectrum transfer tool.
# Invoked by tools/Makefile or standalone: make (from this directory).
# ---------------------------------------------------------------------------

ROOT      ?= $(realpath ../../..)

BUILD_DIR :=  $(ROOT)/build/tools/serial
BIN_DIR   :=  $(ROOT)/bin/tools/serial

CXX       :=  g++
CXXFLAGS  :=  -std=c++20 -O2 -Wall -Wextra

TARGET    :=  $(BIN_DIR)/serial
SRC       :=  serial.cpp

.PHONY: all
all: mkdirs $(TARGET)

.PHONY: mkdirs
mkdirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
