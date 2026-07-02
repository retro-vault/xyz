ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++20 -g
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XTOOLS_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXTOOLS_VERSION=\"$(XTOOLS_VERSION)\"
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
HOST_TRIPLE ?= $(shell $(CXX) -dumpmachine 2>/dev/null || echo unknown)
NEEDS_WINSOCK := 0
ifneq ($(filter Windows_NT MINGW% MSYS% CYGWIN%,$(OS) $(UNAME_S)),)
NEEDS_WINSOCK := 1
endif
ifneq ($(findstring mingw,$(HOST_TRIPLE)),)
NEEDS_WINSOCK := 1
endif
SOCKET_LIBS :=
ifeq ($(NEEDS_WINSOCK),1)
SOCKET_LIBS += -lws2_32
endif
XZ80_DIR ?= $(ROOT)/lib/xz80
XZ80_INCLUDE = -I$(XZ80_DIR)/include
XZ80_LIB = $(PUBLIC_LIB_DIR)/libxz80.a
XGDB_DIR ?= $(ROOT)/lib/xgdb
XGDB_INCLUDE = -I$(XGDB_DIR)/include
RSP_DIR ?= $(ROOT)/lib/rsp
RSP_INCLUDE = -I$(RSP_DIR)/include

HEADERS = $(wildcard $(XGDB_DIR)/include/xgdb/*.h) $(wildcard $(RSP_DIR)/include/rsp/*.h)
OUTPUT_DIR = $(BUILD_DIR)/xc/xgdb
BIN_OUTPUT = $(HOST_BIN_DIR)
LIB_OUTPUT = $(PUBLIC_LIB_DIR)
XGDB_TARGET = $(BIN_OUTPUT)/xgdb
XGDB_SOURCES = $(filter-out src/frontends.cpp src/mi_frontend.cpp,$(wildcard src/*.cpp))
CLI_PROTOCOL_SOURCE = src/frontends.cpp
MI_PROTOCOL_SOURCE = src/mi_frontend.cpp
CLI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/frontends.o
MI_PROTOCOL_OBJECT = $(OUTPUT_DIR)/mi_frontend.o
CLI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxgdb_cli.a
MI_PROTOCOL_LIB = $(LIB_OUTPUT)/libxgdb_mi.a
TEST_DIR = $(ROOT)/tests/tests/xgdb
MI_SMOKE_TEST = $(TEST_DIR)/mi_smoke_test.sh

.PHONY: all test clean

XBFD_DIR    ?= $(ROOT)/lib/xbfd
XBFD_INCLUDE = -I$(XBFD_DIR)/include
XBFD_LIB     = $(PUBLIC_LIB_DIR)/libxbfd.a

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(LIB_OUTPUT)
	$(MAKE) -C $(XZ80_DIR) all ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) -C $(ROOT)/lib/xbfd all ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) -C $(ROOT)/lib/xgdb all ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) -C $(ROOT)/lib/rsp all ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)
	$(MAKE) $(CLI_PROTOCOL_LIB) $(MI_PROTOCOL_LIB)
	$(CXX) $(CXXFLAGS) -pthread $(XGDB_INCLUDE) $(RSP_INCLUDE) $(XBFD_INCLUDE) $(XZ80_INCLUDE) $(XGDB_SOURCES) \
		$(CLI_PROTOCOL_LIB) \
		$(MI_PROTOCOL_LIB) \
		$(PUBLIC_LIB_DIR)/libxgdb.a \
		$(PUBLIC_LIB_DIR)/librsp.a \
		$(XBFD_LIB) \
		$(XZ80_LIB) \
		$(SOCKET_LIBS) \
		-o $(XGDB_TARGET)

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

test: all
	bash $(MI_SMOKE_TEST) $(XGDB_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(CLI_PROTOCOL_LIB) $(MI_PROTOCOL_LIB)
	rm -f $(XGDB_TARGET)
