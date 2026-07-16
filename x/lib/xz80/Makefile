ROOT        ?= ../..
BUILD_DIR   ?= $(ROOT)/build
DIST_DIR    ?= $(ROOT)/bin/x
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX         ?= g++
CXXFLAGS    ?= -std=c++20 -Wall -Wextra -g -Iinclude

OUTPUT_DIR   = $(BUILD_DIR)/lib/xz80
LIB_OUTPUT   = $(PUBLIC_LIB_DIR)
LIB_TARGET   = $(LIB_OUTPUT)/libxz80.a

SOURCES      = src/cpu.cpp src/disassembler.cpp src/formatter.cpp src/memory.cpp
OBJECTS      = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))

TEST_TARGET  = $(OUTPUT_DIR)/xz80_tests
TEST_DIR     = $(ROOT)/tests/tests/xz80
TEST_MAIN    = $(TEST_DIR)/test_main.cpp
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.cpp)

.PHONY: all test clean

all: $(LIB_TARGET)

$(LIB_TARGET): $(OBJECTS) | $(LIB_OUTPUT) $(OUTPUT_DIR)
	ar rcs $@ $(OBJECTS)

$(OUTPUT_DIR)/%.o: src/%.cpp | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(OUTPUT_DIR) $(LIB_OUTPUT)
	$(CXX) $(CXXFLAGS) -I$(TEST_DIR) $(TEST_MAIN) $(SOURCES) \
		-o $(TEST_TARGET)
	$(TEST_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_OUTPUT):
	mkdir -p $(LIB_OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(LIB_TARGET)
