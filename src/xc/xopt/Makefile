ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g

XOPT_DIR := $(ROOT)/lib/xopt
XOPT_INC := $(XOPT_DIR)/include
XOPT_LIB := $(BUILD_DIR)/lib/libxopt.a

INCLUDE := -I$(XOPT_INC)
TARGET := $(HOST_BIN_DIR)/xopt
OBJDIR := $(BUILD_DIR)/xc/xopt/obj

SRCS := src/main.cpp
OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all test clean xopt-lib

all: xopt-lib $(TARGET)

xopt-lib:
	$(MAKE) -C $(XOPT_DIR)

$(TARGET): $(OBJS) $(XOPT_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(XOPT_LIB)

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

$(OBJDIR):
	mkdir -p $@

$(HOST_BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

test: all
	bash tests/xopt_smoke_test.sh $(TARGET)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
