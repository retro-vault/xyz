ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
PUBLIC_INCLUDE_DIR ?= $(DIST_DIR)/include
PUBLIC_LIB_DIR ?= $(DIST_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g

INCLUDE := -Iinclude
OBJDIR := $(BUILD_DIR)/lib/xopt
BUILD_TARGET := $(BUILD_DIR)/lib/libxopt.a
DIST_TARGET := $(PUBLIC_LIB_DIR)/libxopt.a

SOURCES := \
    src/xopt.cpp \
    src/z80cost.cpp \
    src/z80reg.cpp \
    src/z80peep.cpp

OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(SOURCES))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(BUILD_TARGET) $(DIST_TARGET)

$(BUILD_TARGET): $(OBJS) | $(BUILD_DIR)/lib
	ar rcs $@ $^

$(DIST_TARGET): $(BUILD_TARGET) | $(PUBLIC_LIB_DIR)
	cp $< $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/lib:
	mkdir -p $@

$(PUBLIC_LIB_DIR):
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(BUILD_TARGET) $(DIST_TARGET)
