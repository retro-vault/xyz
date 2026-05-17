ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard $(ROOT)/include/xdbgstub/*.hpp)
OUTPUT_DIR = $(BUILD_DIR)/lib/xdbgstub
LIB_DIR = $(PUBLIC_LIB_DIR)
LIB_TARGET = $(LIB_DIR)/libxdbgstub.a
TEST_TARGET = $(OUTPUT_DIR)/xdbgstub_tests

.PHONY: all test clean

all: $(LIB_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(LIB_TARGET): $(OUTPUT_DIR) $(LIB_DIR) $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include -c src/xdbgstub.cpp -o $(OUTPUT_DIR)/xdbgstub.o
	ar rcs $@ $(OUTPUT_DIR)/xdbgstub.o

test: $(OUTPUT_DIR) $(SOURCES) $(HEADERS) tests/test_main.cpp
	$(CXX) $(CXXFLAGS) -pthread -I$(ROOT)/include tests/test_main.cpp src/xdbgstub.cpp -o $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR) $(LIB_DIR)
