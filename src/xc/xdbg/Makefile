ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++20 -g
XZ80_DIR ?= $(ROOT)/lib/xz80
XZ80_INCLUDE = -I$(XZ80_DIR)/include
XZ80_LIB = $(BUILD_DIR)/lib/libxz80.a

HEADERS = $(wildcard $(ROOT)/include/xdbg/*.hpp) $(wildcard $(ROOT)/include/xdbgstub/*.hpp)
OUTPUT_DIR = $(BUILD_DIR)/xc/xdbg
BIN_OUTPUT = $(HOST_BIN_DIR)
LIB_OUTPUT = $(PUBLIC_LIB_DIR)
XDBG_TARGET = $(BIN_OUTPUT)/xdbg
XDBG_Z80_TARGET = $(BIN_OUTPUT)/xdbg-z80
XDBG_SOURCES = $(filter-out src/xdbg_z80.cpp src/frontends.cpp src/mi_frontend.cpp src/dap_frontend.cpp,$(wildcard src/*.cpp))
XDBG_Z80_SOURCES = src/xdbg_z80.cpp
CLI_PROTOCOL_SOURCE = src/frontends.cpp
MI_PROTOCOL_SOURCE = src/mi_frontend.cpp
DAP_PROTOCOL_SOURCE = src/dap_frontend.cpp
CLI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/frontends.o
MI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/mi_frontend.o
DAP_PROTOCOL_OBJECT = $(OUTPUT_DIR)/dap_frontend.o
CLI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxdbg_cli.a
MI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxdbg_mi.a
DAP_PROTOCOL_LIB = $(LIB_OUTPUT)/libxdbg_dap.a

.PHONY: all test clean

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(LIB_OUTPUT)
	$(MAKE) -C $(XZ80_DIR) all
	$(MAKE) -C $(ROOT)/lib/xdbg all
	$(MAKE) -C $(ROOT)/lib/xdbgstub all
	$(MAKE) $(CLI_PROTOCOL_LIB) $(MI_PROTOCOL_LIB) $(DAP_PROTOCOL_LIB)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(XDBG_SOURCES) \
		$(CLI_PROTOCOL_LIB) \
		$(MI_PROTOCOL_LIB) \
		$(DAP_PROTOCOL_LIB) \
		$(PUBLIC_LIB_DIR)/libxdbg.a \
		$(PUBLIC_LIB_DIR)/libxdbgstub.a \
		$(XZ80_LIB) \
		-o $(XDBG_TARGET)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(XZ80_INCLUDE) $(XDBG_Z80_SOURCES) \
		$(PUBLIC_LIB_DIR)/libxdbgstub.a \
		$(XZ80_LIB) \
		-o $(XDBG_Z80_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_OUTPUT):
	mkdir -p $(LIB_OUTPUT)

$(CLI_PROTOCOL_OBJECT): $(CLI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include -c $< -o $@

$(MI_PROTOCOL_OBJECT): $(MI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include -c $< -o $@

$(DAP_PROTOCOL_OBJECT): $(DAP_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include -c $< -o $@

$(CLI_PROTOCOL_LIB): $(LIB_OUTPUT) $(CLI_PROTOCOL_OBJECT)
	ar rcs $@ $(CLI_PROTOCOL_OBJECT)

$(MI_PROTOCOL_LIB): $(LIB_OUTPUT) $(MI_PROTOCOL_OBJECT)
	ar rcs $@ $(MI_PROTOCOL_OBJECT)

$(DAP_PROTOCOL_LIB): $(LIB_OUTPUT) $(DAP_PROTOCOL_OBJECT)
	ar rcs $@ $(DAP_PROTOCOL_OBJECT)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -rf $(LIB_OUTPUT)
	rm -f $(XDBG_TARGET) $(XDBG_Z80_TARGET)
