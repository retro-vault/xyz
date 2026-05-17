ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CC ?= cc
CXX ?= g++
CXXFLAGS ?= -std=c++2a -g
Z80EX_DIR ?= $(ROOT)/lib/z80ex
Z80EX_INCLUDE = -I$(Z80EX_DIR) -I$(Z80EX_DIR)/include
Z80EX_DEFINES = \
	-DWORDS_LITTLE_ENDIAN \
	-DZ80EX_API_REVISION=1 \
	-DZ80EX_VERSION_MAJOR=1 \
	-DZ80EX_VERSION_MINOR=21 \
	-DZ80EX_VERSION_STR=\"1.1.21\" \
	-DZ80EX_RELEASE_TYPE=\"\"

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard $(ROOT)/include/xdbg/*.hpp)
OUTPUT_DIR = $(BUILD_DIR)/lib/xdbg
LIB_DIR = $(PUBLIC_LIB_DIR)
LIB_TARGET = $(LIB_DIR)/libxdbg.a
TEST_TARGET = $(OUTPUT_DIR)/xdbg_tests
OBJECTS = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))
Z80EX_DASM_OBJECT = $(OUTPUT_DIR)/z80ex_dasm.o

.PHONY: all test clean

all: $(LIB_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OUTPUT_DIR)/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -I$(ROOT)/include $(Z80EX_INCLUDE) -c $< -o $@

$(Z80EX_DASM_OBJECT): $(Z80EX_DIR)/z80ex_dasm.c
	$(CC) -g $(Z80EX_INCLUDE) $(Z80EX_DEFINES) -c $< -o $@

$(LIB_TARGET): $(OUTPUT_DIR) $(LIB_DIR) $(OBJECTS) $(Z80EX_DASM_OBJECT)
	ar rcs $@ $(OBJECTS) $(Z80EX_DASM_OBJECT)

test: $(OUTPUT_DIR) $(SOURCES) $(HEADERS) tests/test_main.cpp $(Z80EX_DASM_OBJECT)
	$(CXX) $(CXXFLAGS) -I$(ROOT)/include $(Z80EX_INCLUDE) tests/test_main.cpp $(SOURCES) $(Z80EX_DASM_OBJECT) -o $(TEST_TARGET)
	$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR) $(LIB_DIR)
