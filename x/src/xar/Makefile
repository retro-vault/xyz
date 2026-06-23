ROOT ?= ../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin/x
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a

INCLUDE   := -Iinclude -I$(XBFD_INC)

TARGET    := $(HOST_BIN_DIR)/xar
OBJDIR    := $(BUILD_DIR)/xc/xar/obj

SRCS := \
    src/main.cpp \
    src/cli.cpp \
    src/operations.cpp

OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(XBFD_LIB) $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR)

$(TARGET): $(OBJS) $(XBFD_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

$(OBJDIR):
	mkdir -p $@

$(HOST_BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
