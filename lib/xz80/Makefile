ROOT        ?= ../..
BUILD_DIR   ?= $(ROOT)/build
CXX         ?= g++
CXXFLAGS    ?= -std=c++20 -Wall -Wextra -g -Iinclude

OUTPUT_DIR   = $(BUILD_DIR)/lib/xz80
LIB_OUTPUT   = $(BUILD_DIR)/lib
LIB_TARGET   = $(LIB_OUTPUT)/libxz80.a

SOURCES      = src/cpu.cpp src/disassembler.cpp src/memory.cpp
OBJECTS      = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))

TEST_TARGET  = $(OUTPUT_DIR)/xz80_tests
TEST_MAIN    = tests/test_main.cpp
TEST_SOURCES = $(wildcard tests/test_*.cpp)

.PHONY: all test clean

all: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_OUTPUT) $(OUTPUT_DIR) $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(OUTPUT_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(OUTPUT_DIR) $(LIB_OUTPUT)
	$(CXX) $(CXXFLAGS) -Itests $(TEST_MAIN) $(SOURCES) \
		-o $(TEST_TARGET)
	$(TEST_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_OUTPUT):
	mkdir -p $(LIB_OUTPUT)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(LIB_TARGET)
