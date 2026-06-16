ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -g
XZ80_DIR ?= $(ROOT)/lib/xz80
XZ80_INCLUDE = -I$(XZ80_DIR)/include
XZ80_LIB = $(BUILD_DIR)/lib/libxz80.a
XGDB_DIR ?= $(ROOT)/lib/xgdb
XGDB_INCLUDE = -I$(XGDB_DIR)/include
RSP_DIR ?= $(ROOT)/lib/rsp
RSP_INCLUDE = -I$(RSP_DIR)/include

HEADERS = $(wildcard $(XGDB_DIR)/include/xgdb/*.h) $(wildcard $(RSP_DIR)/include/rsp/*.h)
OUTPUT_DIR = $(BUILD_DIR)/xc/xgdb
BIN_OUTPUT = $(HOST_BIN_DIR)
LIB_OUTPUT = $(PUBLIC_LIB_DIR)
XGDB_TARGET = $(BIN_OUTPUT)/xgdb
XGDB_Z80_TARGET = $(BIN_OUTPUT)/xgdb-z80
XGDB_SOURCES = $(filter-out src/xgdb_z80.cpp src/frontends.cpp src/mi_frontend.cpp,$(wildcard src/*.cpp))
XGDB_Z80_SOURCES = src/xgdb_z80.cpp
CLI_PROTOCOL_SOURCE = src/frontends.cpp
MI_PROTOCOL_SOURCE = src/mi_frontend.cpp
CLI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/frontends.o
MI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/mi_frontend.o
CLI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxgdb_cli.a
MI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxgdb_mi.a

.PHONY: all test clean

XBFD_DIR    ?= $(ROOT)/lib/xbfd
XBFD_INCLUDE = -I$(XBFD_DIR)/include
XBFD_LIB     = $(PUBLIC_LIB_DIR)/libxbfd.a

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(LIB_OUTPUT)
	$(MAKE) -C $(XZ80_DIR) all
	$(MAKE) -C $(ROOT)/lib/xbfd all
	$(MAKE) -C $(ROOT)/lib/xgdb all
	$(MAKE) -C $(ROOT)/lib/rsp all
	$(MAKE) $(CLI_PROTOCOL_LIB) $(MI_PROTOCOL_LIB)
	$(CXX) $(CXXFLAGS) -pthread $(XGDB_INCLUDE) $(RSP_INCLUDE) $(XBFD_INCLUDE) $(XZ80_INCLUDE) $(XGDB_SOURCES) \
		$(CLI_PROTOCOL_LIB) \
		$(MI_PROTOCOL_LIB) \
		$(PUBLIC_LIB_DIR)/libxgdb.a \
		$(PUBLIC_LIB_DIR)/librsp.a \
		$(XBFD_LIB) \
		$(XZ80_LIB) \
		-o $(XGDB_TARGET)
	$(CXX) $(CXXFLAGS) -pthread $(RSP_INCLUDE) $(XZ80_INCLUDE) $(XGDB_Z80_SOURCES) \
		$(PUBLIC_LIB_DIR)/librsp.a \
		$(XZ80_LIB) \
		-o $(XGDB_Z80_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_OUTPUT):
	mkdir -p $(LIB_OUTPUT)

$(CLI_PROTOCOL_OBJECT): $(CLI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread $(XGDB_INCLUDE) $(RSP_INCLUDE) $(XBFD_INCLUDE) $(XZ80_INCLUDE) -c $< -o $@

$(MI_PROTOCOL_OBJECT): $(MI_PROTOCOL_SOURCE)
	$(CXX) $(CXXFLAGS) -pthread $(XGDB_INCLUDE) $(RSP_INCLUDE) $(XBFD_INCLUDE) $(XZ80_INCLUDE) -c $< -o $@

$(CLI_PROTOCOL_LIB): $(LIB_OUTPUT) $(CLI_PROTOCOL_OBJECT)
	ar rcs $@ $(CLI_PROTOCOL_OBJECT)

$(MI_PROTOCOL_LIB): $(LIB_OUTPUT) $(MI_PROTOCOL_OBJECT)
	ar rcs $@ $(MI_PROTOCOL_OBJECT)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -rf $(LIB_OUTPUT)
	rm -f $(XGDB_TARGET) $(XGDB_Z80_TARGET)
