ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
BIN_DIR ?= $(ROOT)/bin
CC ?= cc
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g
Z80EX_DIR ?= $(ROOT)/third_party/z80ex
Z80EX_INCLUDE = -I$(Z80EX_DIR) -I$(Z80EX_DIR)/include
Z80EX_DEFINES = \
	-DWORDS_LITTLE_ENDIAN \
	-DZ80EX_API_REVISION=1 \
	-DZ80EX_VERSION_MAJOR=1 \
	-DZ80EX_VERSION_MINOR=21 \
	-DZ80EX_VERSION_STR=\"1.1.21\" \
	-DZ80EX_RELEASE_TYPE=\"\"

HEADERS = $(wildcard $(ROOT)/include/xdbg/*.hpp) $(wildcard $(ROOT)/include/xdbgstub/*.hpp)
OUTPUT_DIR = $(BUILD_DIR)/xc/xdbg
BIN_OUTPUT = $(BIN_DIR)/xc/xdbg
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
CLI_PROTOCOL_LIB = $(OUTPUT_DIR)/libxdbg_cli.a
MI_PROTOCOL_LIB = $(OUTPUT_DIR)/libxdbg_mi.a
DAP_PROTOCOL_LIB = $(OUTPUT_DIR)/libxdbg_dap.a
Z80EX_CPU_OBJECT = $(OUTPUT_DIR)/z80ex.o

.PHONY: all test clean

all: $(BIN_OUTPUT) $(OUTPUT_DIR)
	$(MAKE) -C $(ROOT)/lib/xdbg all
	$(MAKE) -C $(ROOT)/lib/xdbgstub all
	$(MAKE) $(CLI_PROTOCOL_LIB) $(MI_PROTOCOL_LIB) $(DAP_PROTOCOL_LIB)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(Z80EX_INCLUDE) $(XDBG_SOURCES) \
		$(CLI_PROTOCOL_LIB) \
		$(MI_PROTOCOL_LIB) \
		$(DAP_PROTOCOL_LIB) \
		$(BUILD_DIR)/lib/xdbg/libxdbg.a \
		$(BUILD_DIR)/lib/xdbgstub/libxdbgstub.a \
		-o $(XDBG_TARGET)
	$(CC) -g $(Z80EX_INCLUDE) $(Z80EX_DEFINES) -c $(Z80EX_DIR)/z80ex.c -o $(Z80EX_CPU_OBJECT)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(Z80EX_INCLUDE) $(XDBG_Z80_SOURCES) \
		$(BUILD_DIR)/lib/xdbgstub/libxdbgstub.a \
		$(Z80EX_CPU_OBJECT) \
		-o $(XDBG_Z80_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(CLI_PROTOCOL_OBJECT): $(CLI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(Z80EX_INCLUDE) -c $< -o $@

$(MI_PROTOCOL_OBJECT): $(MI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(Z80EX_INCLUDE) -c $< -o $@

$(DAP_PROTOCOL_OBJECT): $(DAP_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include $(Z80EX_INCLUDE) -c $< -o $@

$(CLI_PROTOCOL_LIB): $(OUTPUT_DIR) $(CLI_PROTOCOL_OBJECT)
	ar rcs $@ $(CLI_PROTOCOL_OBJECT)

$(MI_PROTOCOL_LIB): $(OUTPUT_DIR) $(MI_PROTOCOL_OBJECT)
	ar rcs $@ $(MI_PROTOCOL_OBJECT)

$(DAP_PROTOCOL_LIB): $(OUTPUT_DIR) $(DAP_PROTOCOL_OBJECT)
	ar rcs $@ $(DAP_PROTOCOL_OBJECT)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(XDBG_TARGET) $(XDBG_Z80_TARGET)
