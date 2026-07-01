ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++20 -g
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
RSP_DIR ?= $(ROOT)/lib/rsp
RSP_INCLUDE = -I$(RSP_DIR)/include
RSP_LIB = $(PUBLIC_LIB_DIR)/librsp.a

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard include/xemu/*.h)
OUTPUT_DIR = $(BUILD_DIR)/lib/xemu
LIB_DIR = $(PUBLIC_LIB_DIR)
LIB_TARGET = $(LIB_DIR)/libxemu.a
TEST_TARGET = $(OUTPUT_DIR)/xemu_tests
OBJECTS = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))
TEST_DIR = $(ROOT)/tests/tests/libxemu

.PHONY: all test clean

all: $(LIB_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OUTPUT_DIR)/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -Iinclude $(RSP_INCLUDE) $(XZ80_INCLUDE) -c $< -o $@

$(LIB_TARGET): $(OUTPUT_DIR) $(LIB_DIR) $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(XZ80_LIB):
	$(MAKE) -C $(XZ80_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

$(RSP_LIB):
	$(MAKE) -C $(RSP_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

test: $(OUTPUT_DIR) $(LIB_TARGET) $(XZ80_LIB) $(RSP_LIB) $(TEST_DIR)/test_main.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $(RSP_INCLUDE) $(XZ80_INCLUDE) \
		$(TEST_DIR)/test_main.cpp $(LIB_TARGET) $(RSP_LIB) $(XZ80_LIB) $(SOCKET_LIBS) -pthread -o $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(LIB_TARGET)
