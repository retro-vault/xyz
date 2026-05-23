ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g

SOURCES         = $(wildcard src/*.cpp)
HEADERS         = $(wildcard include/xlink/*.hpp)
TARGET          = xlink
INCLUDE         = -Iinclude -I$(ROOT)/include
XDBG_SOURCE     = $(ROOT)/lib/xdbg/src/xdbg.cpp

TEST_MAIN       = tests/test_main.cpp
TEST_FILTER     = $(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET     = xlink_tests

OUTPUT_DIR      = $(BUILD_DIR)/xc/xlink
BIN_OUTPUT      = $(HOST_BIN_DIR)

.PHONY: all test clean

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(SOURCES) $(HEADERS)
	$(CXX) -o $(BIN_OUTPUT)/$(TARGET) $(SOURCES) $(XDBG_SOURCE) \
		$(CXXFLAGS) $(INCLUDE)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

test: $(OUTPUT_DIR) $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(OUTPUT_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(XDBG_SOURCE) $(CXXFLAGS) $(INCLUDE) -Itests
	$(OUTPUT_DIR)/$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(BIN_OUTPUT)/$(TARGET)
