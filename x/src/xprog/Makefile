ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g
GIT_VERSION_TAG := $(shell git -C $(ROOT) describe --tags --abbrev=0 --match 'v[0-9]*' 2>/dev/null || true)
XPROG_VERSION ?= $(if $(filter v%,$(GIT_VERSION_TAG)),$(patsubst v%,%,$(GIT_VERSION_TAG)),0.1.0)
CXXFLAGS += -DXPROG_VERSION=\"$(XPROG_VERSION)\"

INCLUDE := -Iinclude
TARGET := $(HOST_BIN_DIR)/xprog
OBJDIR := $(BUILD_DIR)/xc/xprog/obj
TEST_BIN := $(BUILD_DIR)/xc/xprog/xprog_tests
TEST_DIR := $(ROOT)/tests/tests/xprog

SRCS := src/main.cpp src/cli.cpp src/package.cpp src/tape.cpp
TEST_SRCS := $(TEST_DIR)/test_main.cpp src/cli.cpp src/package.cpp src/tape.cpp
OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all test clean dirs

all: $(TARGET)

$(TARGET): $(OBJS) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: src/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

dirs:
	@mkdir -p $(OBJDIR) $(dir $(TEST_BIN))

$(HOST_BIN_DIR):
	@mkdir -p $@

test: dirs $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -I$(TEST_DIR) -o $(TEST_BIN) $(TEST_SRCS)
	$(TEST_BIN)

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TEST_BIN) $(TARGET)
