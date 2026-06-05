ROOT        ?= ../..
BUILD_DIR   ?= $(ROOT)/build
DIST_DIR    ?= $(ROOT)/bin
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX         ?= g++
CXXFLAGS    ?= -std=c++20 -g

OUTPUT_DIR  = $(BUILD_DIR)/lib/rsp
LIB_DIR     = $(PUBLIC_LIB_DIR)
LIB_TARGET  = $(LIB_DIR)/librsp.a
SOURCES     = src/rsp.cpp
HEADERS     = $(wildcard include/rsp/*.h)
OBJECTS     = $(OUTPUT_DIR)/rsp.o

.PHONY: all clean

all: $(LIB_TARGET)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

$(OBJECTS): $(SOURCES) $(HEADERS) | $(OUTPUT_DIR)
	$(CXX) $(CXXFLAGS) -pthread -Iinclude -c src/rsp.cpp -o $(OBJECTS)

$(LIB_TARGET): $(LIB_DIR) $(OBJECTS)
	ar rcs $@ $(OBJECTS)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(LIB_TARGET)
