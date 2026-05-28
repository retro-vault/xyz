ROOT ?= ../../..
BUILD_DIR ?= $(ROOT)/build
DIST_DIR ?= $(ROOT)/bin
HOST_BIN_DIR ?= $(DIST_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++2a -Wall -Wextra -Wpedantic -g

XBFD_DIR  := $(ROOT)/lib/xbfd
XBFD_INC  := $(XBFD_DIR)/include
XBFD_LIB  := $(BUILD_DIR)/lib/libxbfd.a

INCLUDE   := -Iinclude -I$(XBFD_INC)

TARGET    := $(HOST_BIN_DIR)/xas
OBJDIR    := $(BUILD_DIR)/xc/xas/obj

SRCS := \
    src/main.cpp \
    src/cli.cpp \
    src/frontend/lexer.cpp \
    src/frontend/parser.cpp \
    src/backend/codegen.cpp \
    src/backend/rel_emitter.cpp \
    src/backend/elf_emitter.cpp

OBJS := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(XBFD_LIB) $(TARGET)

$(XBFD_LIB):
	$(MAKE) -C $(XBFD_DIR)

$(TARGET): $(OBJS) $(XBFD_LIB) | $(HOST_BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: src/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDE) -MMD -MP -c -o $@ $<

dirs:
	@mkdir -p $(sort $(dir $(OBJS)))

$(HOST_BIN_DIR):
	@mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
