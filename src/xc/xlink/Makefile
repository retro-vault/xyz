ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a

SOURCES         = $(wildcard src/*.cpp)
HEADERS         = $(wildcard include/xlink/*.hpp)
TARGET          = xlink
INCLUDE         = -Iinclude -I$(ROOT)/include -I$(XBFD_INC)
XDBG_SOURCE     = $(ROOT)/lib/xdbg/src/xdbg.cpp

TEST_MAIN       = tests/test_main.cpp
TEST_FILTER     = $(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET     = xlink_tests

OUTPUT_DIR      = $(BUILD_DIR)/xc/xlink
BIN_OUTPUT      = $(HOST_BIN_DIR)

.PHONY: all test clean

all: $(XBFD_LIB) $(BIN_OUTPUT) $(OUTPUT_DIR) $(SOURCES) $(HEADERS)
	$(CXX) -o $(BIN_OUTPUT)/$(TARGET) $(SOURCES) $(XDBG_SOURCE) \
		$(CXXFLAGS) $(INCLUDE) $(XBFD_LIB)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

test: $(XBFD_LIB) $(OUTPUT_DIR) $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(OUTPUT_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(XDBG_SOURCE) $(CXXFLAGS) $(INCLUDE) -Itests $(XBFD_LIB)
	$(OUTPUT_DIR)/$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(BIN_OUTPUT)/$(TARGET)
