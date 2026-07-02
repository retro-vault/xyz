ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
DEFAULT_PLATFORM ?= none
CXXFLAGS ?= -std=c++2a -g -DXLD_DEFAULT_PLATFORM=\"$(DEFAULT_PLATFORM)\"
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XLD_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXLD_VERSION=\"$(XLD_VERSION)\"

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XGDB_DIR  := $(ROOT)/lib/xgdb
XGDB_INC  := $(XGDB_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a

SOURCES         = $(wildcard src/*.cpp)
HEADERS         = $(wildcard include/xld/*.h)
TARGET          = xld
INCLUDE         = -Iinclude -I$(XBFD_INC) -I$(XGDB_INC)

TEST_DIR        = $(ROOT)/tests/tests/xld
TEST_MAIN       = $(TEST_DIR)/test_main.cpp
TEST_FILTER     = $(filter-out src/main.cpp,$(SOURCES))
TEST_TARGET     = xld_tests

OUTPUT_DIR      = $(BUILD_DIR)/xc/xld
BIN_OUTPUT      = $(HOST_BIN_DIR)

.PHONY: all test clean xbfd-lib

all: xbfd-lib $(BIN_OUTPUT)/$(TARGET)

$(BIN_OUTPUT)/$(TARGET): $(SOURCES) $(HEADERS) | $(BIN_OUTPUT) $(OUTPUT_DIR)
	$(CXX) -o $@ $(SOURCES) \
		$(CXXFLAGS) $(INCLUDE) $(XBFD_LIB)

xbfd-lib:
	$(MAKE) -C $(XBFD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

$(BIN_OUTPUT):
	mkdir -p $(BIN_OUTPUT)

test: xbfd-lib $(OUTPUT_DIR) $(TEST_MAIN) $(TEST_FILTER) $(HEADERS)
	$(CXX) -o $(OUTPUT_DIR)/$(TEST_TARGET) $(TEST_MAIN) $(TEST_FILTER) \
		$(CXXFLAGS) $(INCLUDE) -I$(TEST_DIR) $(XBFD_LIB)
	$(OUTPUT_DIR)/$(TEST_TARGET)

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f $(BIN_OUTPUT)/$(TARGET)
