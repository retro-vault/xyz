ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
BIN_DIR ?= $(ROOT)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g

SOURCES         = $(wildcard src/*.cpp)
HEADERS         = $(wildcard include/xlink/*.hpp)
TARGET          = xlink
INCLUDE         = -Iinclude

TEST_MAIN       = tests/test_main.cpp
TEST_FILTER     = $(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET     = xlink_tests

OUTPUT_DIR      = $(BUILD_DIR)/xc/xlink
BIN_OUTPUT      = $(BIN_DIR)/xc/xlink

.PHONY: all test clean

all: $(BIN_OUTPUT) $(OUTPUT_DIR) $(SOURCES) $(HEADERS)
	$(CXX) -o $(BIN_OUTPUT)/$(TARGET) $(SOURCES) $(CXXFLAGS) $(INCLUDE)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

test: $(OUTPUT_DIR) $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(OUTPUT_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(CXXFLAGS) $(INCLUDE) -Itests
	$(OUTPUT_DIR)/$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(BIN_OUTPUT)/$(TARGET)
