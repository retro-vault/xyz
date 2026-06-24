ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++20 -g
XEMU_DIR ?= $(ROOT)/lib/xemu
XEMU_INCLUDE = -I$(XEMU_DIR)/include
XEMU_LIB = $(PUBLIC_LIB_DIR)/libxemu.a
XZ80_DIR ?= $(ROOT)/lib/xz80
XZ80_INCLUDE = -I$(XZ80_DIR)/include
XZ80_LIB = $(PUBLIC_LIB_DIR)/libxz80.a
RSP_DIR ?= $(ROOT)/lib/rsp
RSP_INCLUDE = -I$(RSP_DIR)/include
RSP_LIB = $(PUBLIC_LIB_DIR)/librsp.a

HEADERS = $(wildcard $(XEMU_DIR)/include/xemu/*.h) \
	$(wildcard $(RSP_DIR)/include/rsp/*.h) \
	$(wildcard $(XZ80_DIR)/include/xz80/*.h)
OUTPUT_DIR = $(BUILD_DIR)/xc/xemu
BIN_OUTPUT = $(HOST_BIN_DIR)
LIB_OUTPUT = $(PUBLIC_LIB_DIR)
XEMU_TARGET = $(BIN_OUTPUT)/xemu
XEMU_SOURCES = $(wildcard src/*.cpp)
TEST_DIR = $(ROOT)/tests/tests/xemu
STDIO_SMOKE_TEST = $(TEST_DIR)/xemu_stdio_smoke_test.sh

.PHONY: all test clean

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(LIB_OUTPUT) $(XEMU_TARGET)

$(XEMU_TARGET): $(XEMU_SOURCES) $(HEADERS)
	$(MAKE) -C $(ROOT)/lib/xz80 ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) -C $(ROOT)/lib/rsp ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) -C $(ROOT)/lib/xemu ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(CXX) $(CXXFLAGS) -pthread $(XEMU_INCLUDE) $(RSP_INCLUDE) $(XZ80_INCLUDE) $(XEMU_SOURCES) \
		$(XEMU_LIB) \
		$(RSP_LIB) \
		$(XZ80_LIB) \
		-o $(XEMU_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_OUTPUT):
	mkdir -p $(LIB_OUTPUT)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

test: all
	bash $(STDIO_SMOKE_TEST) $(XEMU_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(XEMU_TARGET)
