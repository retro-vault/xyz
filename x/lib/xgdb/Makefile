ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++20 -g
XZ80_DIR ?= $(ROOT)/lib/xz80
XZ80_INCLUDE = -I$(XZ80_DIR)/include
XZ80_LIB = $(PUBLIC_LIB_DIR)/libxz80.a

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard include/xgdb/*.h)
OUTPUT_DIR = $(BUILD_DIR)/lib/xgdb
LIB_DIR = $(PUBLIC_LIB_DIR)
LIB_TARGET = $(LIB_DIR)/libxgdb.a
TEST_TARGET = $(OUTPUT_DIR)/xgdb_tests
OBJECTS = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))
TEST_DIR = $(ROOT)/tests/tests/libxgdb

.PHONY: all test clean

all: $(LIB_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OUTPUT_DIR)/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -Iinclude $(XZ80_INCLUDE) -c $< -o $@

$(LIB_TARGET): $(OUTPUT_DIR) $(LIB_DIR) $(OBJECTS)
	ar rcs $@ $(OBJECTS)

$(XZ80_LIB):
	$(MAKE) -C $(XZ80_DIR) ROOT=$(ROOT) BUILD_DIR=$(BUILD_DIR) DIST_DIR=$(DIST_DIR) PUBLIC_LIB_DIR=$(PUBLIC_LIB_DIR)

test: $(OUTPUT_DIR) $(SOURCES) $(HEADERS) $(TEST_DIR)/test_main.cpp $(XZ80_LIB)
	$(CXX) $(CXXFLAGS) -Iinclude $(XZ80_INCLUDE) \
		$(TEST_DIR)/test_main.cpp $(SOURCES) $(XZ80_LIB) -o $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(LIB_TARGET)
